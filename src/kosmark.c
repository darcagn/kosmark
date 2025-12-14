/* KallistiOS ##version##

   kosmark.c
   Copyright (C) 2025 Eric Fradella

   Based on previous pvrmark examples
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <kos/dbglog.h>
#include <arch/arch.h>
#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>
#include <dc/video.h>
#include <dc/perfctr.h>
#include <stdlib.h>
#include <time.h>

#include "kosmark.h"

/* Our supported primitive types */
typedef enum prim_type {
    PRIM_TYPE_TRI,
    PRIM_TYPE_STRIP,
    PRIM_TYPE_QUAD,
    PRIM_TYPE_MAX
} prim_type_t;

/* Our supported renderers */
typedef enum renderer_type {
    RENDERER_TYPE_DIRECT,
    RENDERER_TYPE_DEFERRED,
    RENDERER_TYPE_GLDC,
    RENDERER_TYPE_MAX
} renderer_type_t;

/* Names for our primitives */
static const char *prim_names[PRIM_TYPE_MAX] = {
    [PRIM_TYPE_TRI]             = "triangles",
    [PRIM_TYPE_STRIP]           = "triangle strips",
    [PRIM_TYPE_QUAD]            = "quadrangles"
};

/* Names for our renderers */
static const char *renderer_names[RENDERER_TYPE_MAX] = {
    [RENDERER_TYPE_DIRECT]      = "direct PVR",
    [RENDERER_TYPE_DEFERRED]    = "deferred PVR",
    [RENDERER_TYPE_GLDC]        = "GLdc"
};

/* Table of function pointers for our frame rendering loops */
static void (*render_frames[RENDERER_TYPE_MAX][PRIM_TYPE_MAX])(void) = {
    [RENDERER_TYPE_DIRECT]      = { direct_tri_frame,   direct_strip_frame,     direct_quad_frame },
    [RENDERER_TYPE_DEFERRED]    = { deferred_tri_frame, deferred_strip_frame,   deferred_quad_frame },
    [RENDERER_TYPE_GLDC]        = { gldc_tri_frame,     gldc_strip_frame,       gldc_quad_frame }
};

/* Our currently selected primitive and renderer types */
static prim_type_t current_prim = DEFAULT_PRIM_TYPE;
static renderer_type_t current_renderer = DEFAULT_RENDERER_TYPE;

#define UP_CLEAR(x) do { \
                        printf("\033[%dA", (x)); \
                        printf("\033[2K"); \
                    } while(0)

#define DOWN(x)   do { \
                        printf("\033[%dB", (x)); \
                    } while(0)

static bool auto_adjust = false;
size_t polycnt = BEGIN_POLYCNT;
static clock_t begin;
static size_t frame_count;
static size_t sample_count;
static size_t missed_frame_count;
static size_t missed_vblank_count;
static size_t missed_frames;
static float avgfps = -1;

static inline void print_framerate(void) {
    if(avgfps > 65)
        return;

    UP_CLEAR(7);
    printf("   average framerate:\t%.2f fps\n", avgfps);
    DOWN(7);
}

static inline void print_poly_target(void) {
    UP_CLEAR(6);
    printf("est. polygons target:\t%.0f per second, %u per frame\n", (float)(polycnt * 59.94), polycnt);
    DOWN(6);
}

static inline void print_polys_rendered(void) {
    size_t polygons_rendered = (size_t)(polycnt * avgfps);
    if(polygons_rendered > (polycnt * 60))
        return;

    UP_CLEAR(5);
    printf("   polygons rendered:\t%u per second, %u per frame\n", (size_t)(polycnt * avgfps), polycnt);
    DOWN(5);
}

static inline void print_frames_missed(size_t num_missed) {
    UP_CLEAR(4);
    printf("       frames missed:\t%u last second\n", num_missed);
    DOWN(4);
}

static inline void print_current_renderer(void) {
    UP_CLEAR(3);
    printf("    current renderer:\t%s\n", renderer_names[current_renderer]);
    DOWN(3);
}

static inline void print_drawing_primitive(void) {
    UP_CLEAR(2);
    printf("   drawing primitive:\t%s\n", prim_names[current_prim]);
    DOWN(2);
}

static inline void print_poly_adjustment(void) {
    UP_CLEAR(1);
    printf("    auto poly adjust:\t%s\n", auto_adjust ? "enabled" : "disabled");
    DOWN(1);
}

static void reset_stats(void) {
    frame_count = 0;
    avgfps = 0;
    sample_count = 0;
    missed_frame_count = 0;
    missed_vblank_count = 0;
    missed_frames = 0;
    begin = clock();
}

static inline void do_stats() {
    static pvr_stats_t stats;
    static clock_t missed_time;

    clock_t now = clock();

    if((now - begin) >= 50000) {
        pvr_get_stats(&stats);

        size_t frame_delta = stats.frame_count - frame_count;

        float current_fps = CLOCKS_PER_SEC * (float)frame_delta / (now - begin);

        if(avgfps <= 0) {
            avgfps = current_fps;
        } else {
            sample_count++;
            avgfps = (avgfps * (sample_count - 1) + current_fps) / sample_count;
        }

        if((now - missed_time) >= CLOCKS_PER_SEC) {
            size_t missed_frame_delta = stats.frame_count - missed_frame_count;
            size_t missed_vblank_delta = stats.vbl_count - missed_vblank_count;

            missed_frames = (missed_vblank_delta - missed_frame_delta);
            missed_time = now;

            missed_frame_count = stats.frame_count;
            missed_vblank_count = stats.vbl_count;
        }

        print_framerate();
        print_poly_target();
        print_polys_rendered();
        print_frames_missed(missed_frames);
        print_current_renderer();
        print_drawing_primitive();
        print_poly_adjustment();

        frame_count = stats.frame_count;

        begin = now;
    }
}

void adjust_polycnt(void) {
    if(missed_frames > 1)
        polycnt -= missed_frames;

    if(missed_frames < 1)
        polycnt++;
}

/* Our PVR initialization parameters */
static pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0 },
    512 * 1024,
};
pvr_poly_hdr_t hdr;
static void renderer_setup(void) {
    switch(current_renderer) {
        case RENDERER_TYPE_DIRECT:
        case RENDERER_TYPE_DEFERRED:
            pvr_init(&pvr_params);
            pvr_set_bg_color(0, 0, 0);

            pvr_poly_cxt_t cxt;
            pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
            cxt.gen.shading = PVR_SHADE_FLAT;
            pvr_poly_compile(&hdr, &cxt);
            break;

        case RENDERER_TYPE_GLDC:
            UP_CLEAR(3);
            glKosInit();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glOrtho(0, 640, 0, 480, -100, 100);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            break;

        default:
            dbglog(DBG_ERROR, "Attempt to setup unknown renderer!\n");
            exit(EXIT_FAILURE);
    }
}

static void renderer_shutdown(void) {
    switch(current_renderer) {
        case RENDERER_TYPE_DIRECT:
        case RENDERER_TYPE_DEFERRED:
            pvr_shutdown();
            break;

        case RENDERER_TYPE_GLDC:
            glKosShutdown();
            break;

        default:
            dbglog(DBG_ERROR, "Attempt to shutdown unknown renderer!\n");
            exit(EXIT_FAILURE);
    }
}

static void handle_input() {
    static uint32_t buttons;
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return;

        if(state->buttons == buttons)
            return;
        else
            buttons = state->buttons;

        // Exit
        if(state->buttons & CONT_START)
            arch_exit();

        /* Switch renderer type */
        if(state->buttons & CONT_A) {
            renderer_shutdown();
            if(++current_renderer == RENDERER_TYPE_MAX)
                current_renderer = 0;
            renderer_setup();
            reset_stats();
        }

        /* Switch primitive type */
        if(state->buttons & CONT_B) {
            if(++current_prim == PRIM_TYPE_MAX)
                current_prim = 0;
            reset_stats();
        }

        /* Reset test */
        if(state->buttons & CONT_X) {
            renderer_shutdown();
            renderer_setup();
            reset_stats();
        }

        /* Toggle automatic framerate adjustment */
        if(state->buttons & CONT_Y) {
            if(auto_adjust)
                auto_adjust = false;
            else
                auto_adjust = true;
        }

        /* Increase polygons by large step */
        if(state->buttons & CONT_DPAD_UP) {
            int new_polycnt = polycnt + POLYCNT_STEP_LG;
            if(new_polycnt > MAX_POLYCNT)
                polycnt = MAX_POLYCNT;
            else
                polycnt = new_polycnt;
            reset_stats();
        }

        /* Increase polygons by small step */
        if(state->buttons & CONT_DPAD_RIGHT) {
            int new_polycnt = polycnt + POLYCNT_STEP_SM;
            if(new_polycnt > MAX_POLYCNT)
                polycnt = MAX_POLYCNT;
            else
                polycnt = new_polycnt;
            reset_stats();
        }

        /* Decrease polygons by large step */
        if(state->buttons & CONT_DPAD_DOWN) {
            int new_polycnt = polycnt - POLYCNT_STEP_LG;
            if(new_polycnt < MIN_POLYCNT)
                polycnt = MIN_POLYCNT;
            else
                polycnt = new_polycnt;
            reset_stats();
        }

        /* Decrease polygons by small step */
        if(state->buttons & CONT_DPAD_LEFT) {
            int new_polycnt = polycnt - POLYCNT_STEP_SM;
            if(new_polycnt < MIN_POLYCNT)
                polycnt = MIN_POLYCNT;
            else
                polycnt = new_polycnt;
            reset_stats();
        }
    }
}

int main(int argc, char **argv) {
    dbglog_set_level(DBG_NOTICE);
    printf("===========================================================\n");
    printf("    kosmark: The KallistiOS graphics renderer benchmark\n");
    printf("===========================================================\n");
    printf("Controls:\n");
    printf("\tStart:\tExit program\n");
    printf("\tA:\tChange rendering type\n");
    printf("\tB:\tChange primitive type\n");
    printf("\tX:\tReset test\n");
    printf("\tY:\tToggle automatic framerate adjustment\n");
    printf("\tUp:\tIncrease polygon rendering target by 1000\n");
    printf("\tRight:\tIncrease polygon rendering target by 100\n");
    printf("\tDown:\tDecrease polygon rendering target by 1000\n");
    printf("\tLeft:\tDecrease polygon rendering target by 100\n");
    printf("===========================================================\n");
    printf("\n\n\n\n\n\n\n\n");

    renderer_setup();

    reset_stats();

    for(;;) {
        render_frames[current_renderer][current_prim]();
        do_stats();
        if(auto_adjust)
            adjust_polycnt();
        handle_input();
    }

    exit(EXIT_SUCCESS);
}
