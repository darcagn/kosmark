#include "kosmark.h"

void direct_tri_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    pvr_vertex_t *vert;
    int x, y;
    float z = 0.0f;
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

void deferred_tri_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    pvr_vertex_t vert;
    int x, y;
    float z;
    int col;

    z = getnum(&seed, 64) + 1;

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    for(size_t i = 0; i < polycnt - 1; i++) {
        get_xyc(&seed, &x, &y, &col);

        vert.flags = PVR_CMD_VERTEX;
        vert.x = x;
        vert.y = y;
        vert.z = z;
        vert.argb = col;
        pvr_prim(&vert, sizeof(vert));

        vert.x = x;
        pvr_prim(&vert, sizeof(vert));

        vert.flags = PVR_CMD_VERTEX_EOL;
        vert.y = y;
        pvr_prim(&vert, sizeof(vert));
    }

    pvr_list_finish();
    pvr_scene_finish();
    oldseed = seed;
}

void gldc_tri_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    int x, y;
    float z;
    int col;
    int size;

    z = getnum(&seed, 64) + 1;
    size = getnum(&seed, 128) + 1;

    glBegin(GL_TRIANGLES);

    for(size_t i = 0; i < polycnt; i++) {
        get_xyc(&seed, &x, &y, &col);

        float x1 = x - size;
        float x2 = x + size;
        float y1 = y - size;
        float y2 = y + size;

        glColor4ubv((void *)&col);
        glVertex3f(x1, y1, z);
        glVertex3f(x2, y1, z);
        glVertex3f(x2, y2, z);
    }

    glEnd();

    glKosSwapBuffers();
    oldseed = seed;
}
