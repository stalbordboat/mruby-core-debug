/*
** state.c - mrb_state open/close functions
**
** See Copyright Notice in mruby.h
*/

#include <stdlib.h>
#include <string.h>
#include <mruby.h>
#include <mruby/irep.h>
#include <mruby/variable.h>
#include <mruby/debug.h>
#include <mruby/string.h>
#include <mruby/class.h>
#include <mruby/internal.h>

void mrb_init_core(mrb_state*);
void mrb_init_mrbgems(mrb_state*);

void mrb_gc_init(mrb_state*, mrb_gc *gc);
void mrb_gc_destroy(mrb_state*, mrb_gc *gc);

int mrb_core_init_protect(mrb_state *mrb, void (*body)(mrb_state*, void*), void *opaque);

static void
init_gc_and_core(mrb_state *mrb, void *opaque)
{
  static const struct mrb_context mrb_context_zero = { 0 };

  mrb_gc_init(mrb, &mrb->gc);
  mrb->c = (struct mrb_context*)mrb_malloc(mrb, sizeof(struct mrb_context));
  *mrb->c = mrb_context_zero;
  mrb->root_c = mrb->c;

  mrb_init_core(mrb);
}

/* Initializes the core of mruby, without loading gems. */
MRB_API mrb_state*
mrb_open_core(void)
{
  static const mrb_state mrb_state_zero = { 0 };
  mrb_state *mrb;

  mrb = (mrb_state*)mrb_basic_alloc_func(NULL, sizeof(mrb_state));
  if (mrb == NULL) return NULL;

  *mrb = mrb_state_zero;
  mrb->atexit_stack_len = 0;
  mrb->bootstrapping = TRUE;

  if (mrb_core_init_protect(mrb, init_gc_and_core, NULL)) {
    mrb_close(mrb);
    return NULL;
  }

  mrb_method_cache_clear(mrb);
  mrb->bootstrapping = FALSE;

  return mrb;
}

#ifndef MRB_NO_GEMS
static void
init_mrbgems(mrb_state *mrb, void *opaque)
{
  mrb_init_mrbgems(mrb);
}
#endif

/* Initializes mruby, including loading gems. */
MRB_API mrb_state*
mrb_open(void)
{
  mrb_state *mrb = mrb_open_core();

  if (mrb == NULL) {
    return NULL;
  }

#ifndef MRB_NO_GEMS
  if (mrb_core_init_protect(mrb, init_mrbgems, NULL)) {
    mrb_close(mrb);
    return NULL;
  }
  mrb_gc_arena_restore(mrb, 0);
#endif
  return mrb;
}

void mrb_free_symtbl(mrb_state *mrb);

void
mrb_irep_incref(mrb_state *mrb, mrb_irep *irep)
{
  if (irep->flags & MRB_IREP_NO_FREE) return;
  if (irep->refcnt == UINT16_MAX) {
    mrb_garbage_collect(mrb);
    if (irep->refcnt == UINT16_MAX) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "too many irep references");
    }
  }
  irep->refcnt++;
}

void
mrb_irep_decref(mrb_state *mrb, mrb_irep *irep)
{
  if (irep->flags & MRB_IREP_NO_FREE) return;
  irep->refcnt--;
  if (irep->refcnt == 0) {
    mrb_irep_free(mrb, irep);
  }
}

void
mrb_irep_cutref(mrb_state *mrb, mrb_irep *irep)
{
  mrb_irep **reps;
  int i;

  if (irep->flags & MRB_IREP_NO_FREE) return;
  reps = (mrb_irep**)irep->reps;
  if (!reps) return;
  for (i=0; i<irep->rlen; i++) {
    mrb_irep *tmp = reps[i];
    reps[i] = NULL;
    if (tmp) mrb_irep_decref(mrb, tmp);
  }
}

void
mrb_irep_free(mrb_state *mrb, mrb_irep *irep)
{
  int i;

  if (irep->flags & MRB_IREP_NO_FREE) return;
  if (!(irep->flags & MRB_ISEQ_NO_FREE))
    mrb_free(mrb, (void*)irep->iseq);
  if (irep->pool) {
    for (i=0; i<irep->plen; i++) {
      if ((irep->pool[i].tt & 3) == IREP_TT_STR ||
          irep->pool[i].tt == IREP_TT_BIGINT) {
        mrb_free(mrb, (void*)irep->pool[i].u.str);
      }
    }
    mrb_free(mrb, (void*)irep->pool);
  }
  mrb_free(mrb, (void*)irep->syms);
  if (irep->reps) {
    for (i=0; i<irep->rlen; i++) {
      if (irep->reps[i])
        mrb_irep_decref(mrb, (mrb_irep*)irep->reps[i]);
    }
    mrb_free(mrb, (void*)irep->reps);
  }
  mrb_free(mrb, (void*)irep->lv);
  mrb_debug_info_free(mrb, irep->debug_info);
#ifdef MRB_DEBUG
  memset(irep, -1, sizeof(*irep));
#endif
  mrb_free(mrb, irep);
}

/* Frees a mruby context. */
MRB_API void
mrb_free_context(mrb_state *mrb, struct mrb_context *c)
{
  if (!c) return;
  mrb_free(mrb, c->stbase);
  mrb_free(mrb, c->cibase);
  mrb_free(mrb, c);
}

void mrb_protect_atexit(mrb_state *mrb);

/* Closes and finalizes a mruby state. */
MRB_API void
mrb_close(mrb_state *mrb)
{
  if (!mrb) return;
  mrb_protect_atexit(mrb);

  /* free */
  mrb_gc_free_gv(mrb);
  mrb_gc_destroy(mrb, &mrb->gc);
  mrb_free_context(mrb, mrb->root_c);
  mrb_free_symtbl(mrb);
  mrb_free(mrb, mrb);
}

/* Adds an instruction sequence (irep) to the mruby state. */
MRB_API mrb_irep*
mrb_add_irep(mrb_state *mrb)
{
  static const mrb_irep mrb_irep_zero = { 0 };
  mrb_irep *irep;

  irep = (mrb_irep*)mrb_malloc(mrb, sizeof(mrb_irep));
  *irep = mrb_irep_zero;
  irep->refcnt = 1;

  return irep;
}

/* Returns the top-level self object. */
MRB_API mrb_value
mrb_top_self(mrb_state *mrb)
{
  return mrb_obj_value(mrb->top_self);
}

/* Registers a function to be called when the mruby state is closed. */
MRB_API void
mrb_state_atexit(mrb_state *mrb, mrb_atexit_func f)
{
#ifdef MRB_FIXED_STATE_ATEXIT_STACK
  if (mrb->atexit_stack_len + 1 > MRB_FIXED_STATE_ATEXIT_STACK_SIZE) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "exceeded fixed state atexit stack limit");
  }
#else
  size_t stack_size;

  stack_size = sizeof(mrb_atexit_func) * (mrb->atexit_stack_len + 1);
  if (mrb->atexit_stack_len == 0) {
    mrb->atexit_stack = (mrb_atexit_func*)mrb_malloc(mrb, stack_size);
  }
  else {
    mrb->atexit_stack = (mrb_atexit_func*)mrb_realloc(mrb, mrb->atexit_stack, stack_size);
  }
#endif

  mrb->atexit_stack[mrb->atexit_stack_len++] = f;
}
