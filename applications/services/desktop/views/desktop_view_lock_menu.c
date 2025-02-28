#include <furi.h>
#include <gui/elements.h>
#include <dolphin/dolphin.h>
#include <assets_icons.h>

#include "../desktop_i.h"
#include "desktop_view_lock_menu.h"
#include "applications/settings/desktop_settings/desktop_settings_app.h"

#define LOCK_MENU_ITEMS_NB 5

typedef enum {
    DesktopLockMenuIndexLock,
    DesktopLockMenuIndexPinLock,
    DesktopLockMenuIndexPinLockShutdown,
    DesktopLockMenuIndexSFW,

    DesktopLockMenuIndexTotalCount
} DesktopLockMenuIndex;

void desktop_lock_menu_set_callback(
    DesktopLockMenuView* lock_menu,
    DesktopLockMenuViewCallback callback,
    void* context) {
    furi_assert(lock_menu);
    furi_assert(callback);
    lock_menu->callback = callback;
    lock_menu->context = context;
}

void desktop_lock_menu_set_pin_state(DesktopLockMenuView* lock_menu, bool pin_is_set) {
    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        { model->pin_is_set = pin_is_set; },
        true);
}

void desktop_lock_menu_set_sfw_mode_state(DesktopLockMenuView* lock_menu, bool sfw_mode) {
    with_view_model(
        lock_menu->view, DesktopLockMenuViewModel * model, { model->sfw_mode = sfw_mode; }, true);
}

void desktop_lock_menu_set_idx(DesktopLockMenuView* lock_menu, uint8_t idx) {
    furi_assert(idx < DesktopLockMenuIndexTotalCount);
    with_view_model(
        lock_menu->view, DesktopLockMenuViewModel * model, { model->idx = idx; }, true);
}

void desktop_lock_menu_draw_callback(Canvas* canvas, void* model) {
    DesktopLockMenuViewModel* m = model;

    canvas_set_color(canvas, ColorBlack);
    canvas_draw_icon(canvas, -57, 0 + STATUS_BAR_Y_SHIFT, &I_DoorLeft_70x55);
    canvas_draw_icon(canvas, 116, 0 + STATUS_BAR_Y_SHIFT, &I_DoorRight_70x55);
    canvas_set_font(canvas, FontBatteryPercent);

    for(size_t i = 0; i < DesktopLockMenuIndexTotalCount; ++i) {
        const char* str = NULL;

        if(i == DesktopLockMenuIndexLock) {
            str = "Lock";
        } else if(i == DesktopLockMenuIndexPinLock) {
            if(m->pin_is_set) {
                str = "Lock with PIN";
            } else {
                str = "Set PIN";
            }
        } else if(i == DesktopLockMenuIndexPinLockShutdown) {
            if(m->pin_is_set) {
                str = "Lock with PIN + Off";
            } else {
                str = "Set PIN + Off";
            }
        } else if(i == DesktopLockMenuIndexSFW) {
            if(m->sfw_mode) {
                str = "NSFW Mode";
            } else {
                str = "SFW Mode";
            }
        }

        if(str) //-V547
            canvas_draw_str_aligned(
                canvas, 64, 9 + (i * 12) + STATUS_BAR_Y_SHIFT, AlignCenter, AlignCenter, str);

        if(m->idx == i) elements_frame(canvas, 15, 1 + (i * 12) + STATUS_BAR_Y_SHIFT, 98, 15);
    }
}

View* desktop_lock_menu_get_view(DesktopLockMenuView* lock_menu) {
    furi_assert(lock_menu);
    return lock_menu->view;
}

bool desktop_lock_menu_input_callback(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    DesktopLockMenuView* lock_menu = context;
    uint8_t idx = 0;
    bool consumed = false;
    bool sfw_mode = false;
    bool update = false;

    with_view_model(
        lock_menu->view,
        DesktopLockMenuViewModel * model,
        {
            if((event->type == InputTypeShort) || (event->type == InputTypeRepeat)) {
                if(event->key == InputKeyUp) {
                    if(model->idx == 0) {
                        model->idx = DesktopLockMenuIndexTotalCount - 1;
                    } else {
                        model->idx = CLAMP(model->idx - 1, DesktopLockMenuIndexTotalCount - 1, 0);
                    }
                    update = true;
                    consumed = true;
                } else if(event->key == InputKeyDown) {
                    if(model->idx == DesktopLockMenuIndexTotalCount - 1) {
                        model->idx = 0;
                    } else {
                        model->idx = CLAMP(model->idx + 1, DesktopLockMenuIndexTotalCount - 1, 0);
                    }
                    update = true;
                    consumed = true;
                }
            }
            idx = model->idx;
            sfw_mode = model->sfw_mode;
        },
        update);

    if(event->key == InputKeyOk) {
        if((idx == DesktopLockMenuIndexLock) && (event->type == InputTypeShort)) {
            lock_menu->callback(DesktopLockMenuEventLock, lock_menu->context);
        } else if((idx == DesktopLockMenuIndexPinLock) && (event->type == InputTypeShort)) {
            lock_menu->callback(DesktopLockMenuEventPinLock, lock_menu->context);
        } else if((idx == DesktopLockMenuIndexPinLockShutdown) && (event->type == InputTypeShort)) {
            lock_menu->callback(DesktopLockMenuEventPinLockShutdown, lock_menu->context);
            // } else if((idx == DesktopLockMenuIndexGameMode) && (event->type == InputTypeShort)) {
            // desktop_view_lock_menu_sfwmode_changed(1);
            // DOLPHIN_DEED(getRandomDeed());
            // lock_menu->callback(DesktopLockMenuEventExit, lock_menu->context);
        } else if(idx == DesktopLockMenuIndexSFW) {
            // DOLPHIN_DEED(getRandomDeed());
            if((sfw_mode == false) && (event->type == InputTypeShort)) {
                lock_menu->callback(DesktopLockMenuEventSFWModeOn, lock_menu->context);
            } else if((sfw_mode == true) && (event->type == InputTypeShort)) {
                lock_menu->callback(DesktopLockMenuEventSFWModeOff, lock_menu->context);
            }
        }
        consumed = true;
    }

    return consumed;
}

DesktopLockMenuView* desktop_lock_menu_alloc() {
    DesktopLockMenuView* lock_menu = malloc(sizeof(DesktopLockMenuView));
    lock_menu->view = view_alloc();
    view_allocate_model(lock_menu->view, ViewModelTypeLocking, sizeof(DesktopLockMenuViewModel));
    view_set_context(lock_menu->view, lock_menu);
    view_set_draw_callback(lock_menu->view, (ViewDrawCallback)desktop_lock_menu_draw_callback);
    view_set_input_callback(lock_menu->view, desktop_lock_menu_input_callback);

    return lock_menu;
}

void desktop_lock_menu_free(DesktopLockMenuView* lock_menu_view) {
    furi_assert(lock_menu_view);

    view_free(lock_menu_view->view);
    free(lock_menu_view);
}
