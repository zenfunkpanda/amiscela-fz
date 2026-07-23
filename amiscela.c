#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

typedef struct {
    FuriMessageQueue* event_queue;
    ViewPort* view_port;
    Gui* gui;
    uint16_t gasoline_deciliters;
    uint8_t oil_percent;
} AmiscelaApp;

static void amiscela_draw_callback(Canvas* canvas, void* context) {
    AmiscelaApp* app = context;
    char liters_text[16];
    char percent_text[8];
    char oil_text[20];
    const uint16_t oil_ml = app->gasoline_deciliters * app->oil_percent;
    /* Densita media adottata per olio da miscela: 0,90 g/ml. */
    const uint16_t oil_grams = (oil_ml * 90 + 50) / 100;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    snprintf(
        liters_text,
        sizeof(liters_text),
        "< %u.%u L >",
        app->gasoline_deciliters / 10,
        app->gasoline_deciliters % 10);
    snprintf(percent_text, sizeof(percent_text), "%u%%", app->oil_percent);
    snprintf(oil_text, sizeof(oil_text), "Olio: %u gr", oil_grams);

    /* Textbox dei valori, con indicazione dei tasti associati. */
    canvas_draw_frame(canvas, 2, 1, 125, 54);
    canvas_draw_str_aligned(canvas, 64, 12, AlignCenter, AlignBottom, "Benzina");
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 29, AlignCenter, AlignBottom, liters_text);
    canvas_set_font(canvas, FontSecondary);
    /* Frecce per indicare la regolazione della percentuale. */
    canvas_draw_line(canvas, 45, 50, 45, 40);
    canvas_draw_line(canvas, 45, 40, 41, 44);
    canvas_draw_line(canvas, 45, 40, 49, 44);
    canvas_draw_line(canvas, 57, 40, 57, 50);
    canvas_draw_line(canvas, 57, 50, 53, 46);
    canvas_draw_line(canvas, 57, 50, 61, 46);
    canvas_draw_str_aligned(canvas, 85, 50, AlignRight, AlignBottom, percent_text);

    canvas_draw_str(canvas, 4, 63, oil_text);
    canvas_draw_str_aligned(canvas, 126, 63, AlignRight, AlignBottom, "BACK: esci");
}

static void amiscela_input_callback(InputEvent* input_event, void* context) {
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

int32_t amiscela_app(void* context) {
    UNUSED(context);

    AmiscelaApp app = {0};
    app.gasoline_deciliters = 10;
    app.oil_percent = 3;
    app.event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app.view_port = view_port_alloc();

    view_port_draw_callback_set(app.view_port, amiscela_draw_callback, &app);
    view_port_input_callback_set(app.view_port, amiscela_input_callback, app.event_queue);

    app.gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app.gui, app.view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;

    while(running &&
          furi_message_queue_get(app.event_queue, &event, FuriWaitForever) == FuriStatusOk) {
        if(event.key == InputKeyBack && event.type == InputTypeShort) {
            running = false;
        } else if(
            event.key == InputKeyRight &&
            (event.type == InputTypeShort || event.type == InputTypeLong)) {
            const uint16_t step = event.type == InputTypeLong ? 10 : 1;
            if(app.gasoline_deciliters + step <= 500) {
                app.gasoline_deciliters += step;
            } else {
                app.gasoline_deciliters = 500;
            }
            view_port_update(app.view_port);
        } else if(
            event.key == InputKeyLeft &&
            (event.type == InputTypeShort || event.type == InputTypeLong)) {
            const uint16_t step = event.type == InputTypeLong ? 10 : 1;
            if(app.gasoline_deciliters > step) {
                app.gasoline_deciliters -= step;
            } else {
                app.gasoline_deciliters = 1;
            }
            view_port_update(app.view_port);
        } else if(event.key == InputKeyUp && event.type == InputTypeShort) {
            if(app.oil_percent < 6) app.oil_percent++;
            view_port_update(app.view_port);
        } else if(event.key == InputKeyDown && event.type == InputTypeShort) {
            if(app.oil_percent > 3) app.oil_percent--;
            view_port_update(app.view_port);
        }
    }

    gui_remove_view_port(app.gui, app.view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app.view_port);
    furi_message_queue_free(app.event_queue);

    return 0;
}
