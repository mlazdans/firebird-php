#include <firebird/fb_c_api.h>

#include "php.h"
#include "php_firebird.h"
#include "php_firebird_includes.h"
#include "zend_fibers.h"

zend_class_entry *FireBird_Event_ce;
static zend_object_handlers FireBird_Event_object_handlers;

PHP_METHOD(Event, __construct)
{
}

PHP_METHOD(Event, consume)
{
    ISC_STATUS_ARRAY status;
    firebird_event *event = fb_events.events;

    while (event) {
        // FBDEBUG_NOFL("  consume: %s, count: %d", event->name, event->posted_count);
        if (event->posted_count > 0) {
            zval args;
            ZVAL_LONG(&args, event->posted_count);

            ZVAL_NULL(&event->retval);
            event->fci.retval = &event->retval;
            event->fci.param_count = 1;
            event->fci.params = &args;
            // ZVAL_COPY(event->fci.params, &args);
            // ZVAL_COPY_VALUE(event->fci.params, &args[]);

            // FBDEBUG("  fci: %p, fcc: %p", &event->fci, &event->fcc);
            if (FAILURE == zend_call_function(&event->fci, &event->fcc)) {
                _php_firebird_module_fatal("Failed to call function");
                RETURN_FALSE;
            }
        }

        event = event->next;
    }

    event = fb_events.events;
    while (event) {
        if (event->posted_count > 0) {
            event->posted_count = 0;
            if (isc_que_events(status, event->db_handle, &event->event_id, event->buff_len, event->event_buffer, event_ast_routine, NULL)) {
                update_err_props(status, FireBird_Event_ce, ZEND_THIS);
                RETURN_FALSE;
            }
        }

        event = event->next;
    }

    RETURN_TRUE;
}

const zend_function_entry FireBird_Event_methods[] = {
    PHP_ME(Event, __construct, arginfo_none, ZEND_ACC_PRIVATE)
    // PHP_ME(Event, resume, arginfo_none_return_bool, ZEND_ACC_PUBLIC)
    PHP_ME(Event, consume, arginfo_none_return_bool, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};

static zend_object *FireBird_Event_create(zend_class_entry *ce)
{
    FBDEBUG("FireBird_Event_create()");

    firebird_event *s = zend_object_alloc(sizeof(firebird_event), ce);

    zend_object_std_init(&s->std, ce);
    object_properties_init(&s->std, ce);

    return &s->std;
}

static void FireBird_Event_free_obj(zend_object *obj)
{
    FBDEBUG("FireBird_Event_free_obj");

    firebird_event *event = Z_EVENT_O(obj);

    zend_object_std_dtor(&event->std);
}

void register_FireBird_Event_ce()
{
    zend_class_entry tmp_ce;
    INIT_NS_CLASS_ENTRY(tmp_ce, "FireBird", "Event", FireBird_Event_methods);
    FireBird_Event_ce = zend_register_internal_class(&tmp_ce);

    // DECLARE_PROP_OBJ(FireBird_Event_ce, transaction, FireBird\\Transaction, ZEND_ACC_PROTECTED_SET);
    DECLARE_ERR_PROPS(FireBird_Event_ce);

    zend_class_implements(FireBird_Event_ce, 1, FireBird_IError_ce);

    FireBird_Event_ce->create_object = FireBird_Event_create;
    FireBird_Event_ce->default_object_handlers = &FireBird_Event_object_handlers;

    memcpy(&FireBird_Event_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

    FireBird_Event_object_handlers.offset = XtOffsetOf(firebird_event, std);
    FireBird_Event_object_handlers.free_obj = FireBird_Event_free_obj;
}

firebird_events fb_events = {0};

// TODO: most likely won't fork in threaded environment or with multi database connections
void event_ast_routine(void *__ev, ISC_USHORT length, const ISC_UCHAR *result_buffer)
{
    firebird_event *current_event = NULL;
    firebird_event *p = fb_events.events;
    const ISC_UCHAR *event_str = result_buffer + 2; // Not sure if this is a good idead. API might change someday and this will break
    ISC_UCHAR event_strlen = result_buffer[1];
    size_t i = 0;

    while (p) {
        if (!memcmp(p->name, event_str, event_strlen)) {
            current_event = p;
            break;
        }

        i++;
        p = p->next;
    }

    if (current_event == NULL) {
        FBDEBUG("Event %.*s not found", event_strlen, event_str);
        return;
    }

    ISC_STATUS_ARRAY status;
    ISC_ULONG counts[1] = { 0 };

    current_event->buff_len = length;
    if(length > 11) {
        _php_firebird_module_fatal("length > 11");
        return;
    }

    // memcpy(current_event->result_buffer, result_buffer, length);

    isc_event_counts(counts, length, current_event->event_buffer, result_buffer);

    // php_printf("buff: " "%" LL_MASK "d" ", l: %d\n", counts[0], length);
    // dump_buffer((char *)&counts, sizeof(counts));

    if (current_event->state == NEW) {
        current_event->state = ACTIVE;
        if (isc_que_events(status, current_event->db_handle, &current_event->event_id, length, current_event->event_buffer, event_ast_routine, NULL)) {
            status_fbp_error(status);
        }
    } else if (current_event->state == ACTIVE) {
        current_event->posted_count += counts[0];
        // ZVAL_NULL(&current_event->retval);
        // current_event->fci.retval = &current_event->retval;

        // FBDEBUG("  fci: %p, fcc: %p", &current_event->fci, &current_event->fcc);
        // if (zend_call_function(&current_event->fci, &current_event->fcc) == SUCCESS) {
        //     php_var_dump(&current_event->retval, 1);
        // } else {
        //     php_error_docref(NULL, E_ERROR, "Failed to call function");
        // }

        // zend_fiber *fiber = current_event->fiber;
        // FBDEBUG_NOFL("  fiber %p, %p", fiber, current_event->fiber);

        // if (fiber->context.status == ZEND_FIBER_STATUS_SUSPENDED) {
        //     FBDEBUG("zend_fiber_resume");
        //     zend_fiber_resume(fiber, NULL, &current_event->retval);
        //     FBDEBUG("zend_fiber_resume returned type: %d, fiber_status: %d", Z_TYPE(current_event->retval), fiber->context.status);
        // } else if (fiber->context.status == ZEND_FIBER_STATUS_INIT) {
        //     FBDEBUG("zend_fiber_start");
        //     fiber->fci.params = &current_event->instance;
        //     fiber->fci.param_count = 1;

        //     if(FAILURE == zend_fiber_start(fiber, &current_event->retval)) {
        //         _php_firebird_module_fatal("Fiber start failed");
        //     } else {
        //         FBDEBUG("zend_fiber_start returned type: %d, fiber_status: %d", Z_TYPE(current_event->retval), fiber->context.status);
        //     }
        // } else if (fiber->context.status == ZEND_FIBER_STATUS_DEAD) {
        //     _php_firebird_module_fatal("ZEND_FIBER_STATUS_DEAD");
        // } else {
        //     _php_firebird_module_fatal("BUG! Unexpected context.status: %d", fiber->context.status);
        // }

        // if (fiber->context.status == ZEND_FIBER_STATUS_DEAD) {
        //     _php_firebird_module_fatal("ZEND_FIBER_STATUS_DEAD2");
        // } else {
        //     if (isc_que_events(status, current_event->db_handle, &current_event->event_id, length, current_event->event_buffer, event_ast_routine, NULL)) {
        //         status_fbp_error(status);
        //     }
        // }
    }
}
