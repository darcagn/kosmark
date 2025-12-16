#include "kosmark.h"

void direct_strip_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    pvr_vertex_t *vert;
    int x, y, col;
    float z;
    pvr_dr_state_t dr_state;

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    pvr_dr_init(&dr_state);

    z = getnum(&seed, 128) + 10;

    get_xyc(&seed, &x, &y, &col);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = x;
    vert->y = y;
    vert->z = z;
    vert->argb = col;
    pvr_dr_commit(vert);

    get_xyc(&seed, &x, &y, &col);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = x;
    vert->y = y;
    vert->z = z;
    vert->argb = col;
    pvr_dr_commit(vert);

    for(size_t i = 0; i < polycnt - 1; i++) {
        get_xyc(&seed, &x, &y, &col);

        vert = pvr_dr_target(dr_state);
        vert->x = x;
        vert->y = y;
        vert->argb = col;
        pvr_dr_commit(vert);
    }

    get_xyc(&seed, &x, &y, &col);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX_EOL;
    vert->x = x;
    vert->y = y;
    vert->argb = col;
    pvr_dr_commit(vert);

    pvr_list_finish();
    pvr_scene_finish();
    oldseed = seed;
}

void deferred_strip_frame() {
    static int oldseed = 0xdeadbeef;
    int seed = oldseed;
    pvr_vertex_t vert;
    int x, y, col;

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    vert.z = getnum(&seed, 128) + 10;

    for(size_t i = 0; i < polycnt; i++) {
        get_xyc(&seed, &x, &y, &col);

        vert.flags = PVR_CMD_VERTEX;
        vert.x = x;
        vert.y = y;
        vert.argb = col;
        pvr_prim(&vert, sizeof(vert));
    }

    get_xyc(&seed, &x, &y, &col);

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x;
    vert.y = y;
    vert.argb = col;
    pvr_prim(&vert, sizeof(vert));

    pvr_list_finish();
    pvr_scene_finish();
    oldseed = seed;
}

void gldc_strip_frame() {
    static int oldseed = 0xdeadbeef;
    static GLfloat verts[MAX_POLYCNT * 3];
    static GLubyte colors[MAX_POLYCNT * 4];
    
    int seed = oldseed;
    int x = 512, y = 256;
    int col;
    float z = getnum(&seed, 128) + 10;

    for(size_t i = 0; i < polycnt; i++) {
        get_xyc(&seed, &x, &y, &col);
        
        verts[i * 3 + 0] = x;
        verts[i * 3 + 1] = y;
        verts[i * 3 + 2] = z;
        
        colors[i * 4 + 0] = (col >> 16) & 0xFF;
        colors[i * 4 + 1] = (col >> 8) & 0xFF;
        colors[i * 4 + 2] = col & 0xFF;
        colors[i * 4 + 3] = 0xFF;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, 0, colors);    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, polycnt);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glKosSwapBuffers();
    oldseed = seed;
}
