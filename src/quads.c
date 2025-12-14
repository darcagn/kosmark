#include "kosmark.h"

void direct_quad_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    pvr_vertex_t *vert;
    int x, y;
    float z;
    int col;
    pvr_dr_state_t dr_state;

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    pvr_dr_init(&dr_state);

    z = getnum(&seed, 64) + 1;

    for(size_t i = 0; i < polycnt; i++) {
        get_xyc(&seed, &x, &y, &col);

        vert = pvr_dr_target(dr_state);
        vert->flags = PVR_CMD_VERTEX;
        vert->x = x;
        vert->y = y;
        vert->z = z;
        vert->argb = col;
        pvr_dr_commit(vert);

        get_xc(&seed, &x, &col);

        vert = pvr_dr_target(dr_state);
        vert->flags = PVR_CMD_VERTEX;
        vert->x = x;
        vert->y = y;
        vert->z = z;
        vert->argb = col;
        pvr_dr_commit(vert);

        get_yc(&seed, &y, &col);

        vert = pvr_dr_target(dr_state);
        vert->flags = PVR_CMD_VERTEX_EOL;
        vert->x = x;
        vert->y = y;
        vert->z = z;
        vert->argb = col;
        pvr_dr_commit(vert);
    }

    pvr_list_finish();
    pvr_scene_finish();
    oldseed = seed;
}

void deferred_quad_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    pvr_vertex_t vert;
    int x, y, col;

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    vert.z = getnum(&seed, 128) + 1;

    for(size_t i = 0; i < polycnt; i++) {
        get_xyc(&seed, &x, &y, &col);

        vert.flags = PVR_CMD_VERTEX;
        vert.x = x;
        vert.y = y;
        vert.argb = col;
        pvr_prim(&vert, sizeof(vert));

        get_xyc(&seed, &x, &y, &col);

        vert.flags = PVR_CMD_VERTEX;
        vert.x = x;
        vert.y = y;
        vert.argb = col;
        pvr_prim(&vert, sizeof(vert));

        get_xyc(&seed, &x, &y, &col);

        vert.flags = PVR_CMD_VERTEX;
        vert.x = x;
        vert.y = y;
        vert.argb = col;
        pvr_prim(&vert, sizeof(vert));

        get_xyc(&seed, &x, &y, &col);

        vert.flags = PVR_CMD_VERTEX_EOL;
        vert.x = x;
        vert.y = y;
        vert.argb = col;
        pvr_prim(&vert, sizeof(vert));
    }

    pvr_list_finish();
    pvr_scene_finish();
    oldseed = seed;
}

void gldc_quad_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    int x, y;
    float z;
    int col;

    z = getnum(&seed, 128) + 1;

    glBegin(GL_TRIANGLE_STRIP);

    for(size_t i = 0; i < polycnt; i++) {
        get_xyc(&seed, &x, &y, &col);

        glColor4ubv((void *)&col);
        glVertex3f(x, y, z);

        get_xy(&seed, &x, &y);

        glColor4ubv((void *)&col);
        glVertex3f(x, y, z);

        get_xy(&seed, &x, &y);

        glColor4ubv((void *)&col);
        glVertex3f(x, y, z);

        get_xy(&seed, &x, &y);

        glColor4ubv((void *)&col);
        glVertex3f(x, y, z);
    }

    glEnd();

    glKosSwapBuffers();
    oldseed = seed;
}
