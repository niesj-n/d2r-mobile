#pragma once
/* Android-only rendering feedback and a visible hold-cancel target. The shared
 * state machine still owns hold timing, right-stick aim, ZL lead and pulses.
 * This file never schedules a cast and never classifies a skill. */
typedef struct {
    int active, target;
    float x0,y0,x,y;
} android_cancelled_aim;
static android_cancelled_aim cancelled_aim[DM_TC_MAX_TOUCH];
static dm_tc_target aim_cached_circle;
static int aim_circle_cached;
static dm_tc_target quick_circle(int index);

static int aim_feedback_gameplay(void) {
    return focused && ready && !cancel && !resizing && scene.game && !frontend && !tc.navigation_mode && dm_uiscreen_ingame();
}
static void aim_feedback_reset(void) { memset(cancelled_aim,0,sizeof cancelled_aim); aim_circle_cached=0; }
static void aim_feedback_sync(void) {
    /* The shared renderer retains its legacy map cancel square for other
     * platforms. Android hides that map target and uses the visible circle. */
    tc.layout.target[DM_TC_MAP].r=aim_feedback_gameplay()?0:22*dm_tc_scale_for_size(width,height);
}
static int aim_feedback_circle_clear(float x,float y,float r,float k) {
    for(int i=0;i<DM_TC_COUNT;++i) {
        if(i==DM_TC_MAP) continue;
        dm_tc_target t=tc.layout.target[i];
        if(t.r>0 && hypotf(x-t.x,y-t.y)<r+t.r+8*k) return 0;
    }
    dm_tc_target t=tc.layout.swap;
    if(hypotf(x-t.x,y-t.y)<r+t.r+8*k) return 0;
    for(int i=0;i<DM_ANDROID_TOUCH_QUICK_COUNT;++i) {
        t=quick_circle(i);
        if(hypotf(x-t.x,y-t.y)<r+t.r+8*k) return 0;
    }
    /* Reserve all upper belt rows even while collapsed: opening the belt must
     * not move the cancel target under a held skill finger. */
    float u=tc.layout.height/1320.0f;
    for(int row=1;row<4;++row) for(int col=0;col<4;++col) {
        t=tc.layout.target[DM_TC_BELT1+col];
        float by=t.y-(165+162*(row-1))*u;
        float dx=fmaxf(0,fabsf(x-t.x)-60*u),dy=fmaxf(0,fabsf(y-by)-53.75f*u);
        if(hypotf(dx,dy)<r+8*k) return 0;
    }
    if(monitor_right>monitor_left && monitor_bottom>monitor_top &&
       x+r+8*k>monitor_left && x-r-8*k<monitor_right &&
       y+r+8*k>monitor_top && y-r-8*k<monitor_bottom) return 0;
    return hypotf(x-tc.layout.joy_x,y-tc.layout.joy_y)>r+tc.layout.joy_radius+8*k;
}
static dm_tc_target aim_feedback_circle_compute(void) {
    float k=dm_tc_scale_for_size(width,height), r=28*k;
    float min_x=safe_left+r+8*k, max_x=width-safe_right-r-8*k;
    float min_y=safe_top+r+8*k, max_y=height-safe_bottom-r-8*k;
    float want_x=tc.layout.target[DM_TC_MAIN].x-170*k;
    float want_y=tc.layout.target[DM_TC_MAIN].y-100*k;
    dm_tc_target best={fminf(max_x,fmaxf(min_x,want_x)),fminf(max_y,fmaxf(min_y,want_y)),r,0};
    float score=INFINITY;
    /* A stable free circle above/left of the skill fan. Java draws these exact
     * coordinates; no separate UI hit target can drift away from the graphic. */
    for(int yi=0;yi<15;++yi) for(int xi=0;xi<9;++xi) {
        float x=fminf(max_x,fmaxf(min_x,want_x+(xi-4)*32*k));
        float y=fminf(max_y,fmaxf(min_y,want_y+(yi-7)*28*k));
        float d=hypotf(x-want_x,y-want_y);
        if(d<score && aim_feedback_circle_clear(x,y,r,k)) { best.x=x; best.y=y; score=d; }
    }
    return best;
}
static dm_tc_target aim_feedback_circle(void) {
    if(!aim_circle_cached) { aim_cached_circle=aim_feedback_circle_compute(); aim_circle_cached=1; }
    return aim_cached_circle;
}
static int aim_feedback_event(const touch_event *e) {
    android_cancelled_aim *c=&cancelled_aim[e->slot];
    if(e->phase==0) { memset(c,0,sizeof *c); return 0; }
    if(c->active) {
        c->x=e->x; c->y=e->y;
        if(e->phase==2 || e->phase==3) memset(c,0,sizeof *c);
        return 1; /* An already cancelled finger stays owned until its UP. */
    }
    dm_tc_finger *f=&tc.finger[e->slot];
    if(!aim_feedback_gameplay() || !f->active || f->target<DM_TC_MAIN || f->target>DM_TC_SKILL4 ||
       (e->phase!=1 && e->phase!=2)) return 0;
    dm_tc_target t=aim_feedback_circle();
    if(hypotf(e->x-t.x,e->y-t.y)>t.r) return 0;
    *c=(android_cancelled_aim){e->phase!=2,f->target,f->aim_origin_x,f->aim_origin_y,e->x,e->y};
    dm_tc_cancel_finger(&tc,e->slot); /* No end_at: cancellation never makes up a short-tap pulse. */
    return 1;
}
static int aim_feedback_active(void) {
    for(int i=0;i<DM_TC_MAX_TOUCH;++i) if(cancelled_aim[i].active ||
       (tc.finger[i].active && tc.finger[i].target>=DM_TC_MAIN && tc.finger[i].target<=DM_TC_SKILL4)) return 1;
    return 0;
}
static void aim_feedback_layout(float *out) {
    if(!aim_feedback_gameplay()) return;
    float k=dm_tc_scale_for_size(width,height);
    for(int i=0;i<DM_TC_MAX_TOUCH;++i) {
        const dm_tc_finger *f=&tc.finger[i]; const android_cancelled_aim *c=&cancelled_aim[i];
        int target=c->active?c->target:f->target;
        if(!(c->active || (f->active && !f->aim_cancelled)) || target<DM_TC_MAIN || target>DM_TC_SKILL4) continue;
        float dx=c->active?c->x-c->x0:f->x-f->aim_origin_x;
        float dy=c->active?c->y-c->y0:f->y-f->aim_origin_y;
        float d=hypotf(dx,dy), limit=18*k, n=d>limit?limit/d:1;
        float *p=out+DM_ANDROID_TOUCH_AIM+target*6;
        p[0]=1; p[1]=dx*n; p[2]=dy*n;
        /* Match shared update_aim's per-axis saturation. Long diagonal drags
         * must point where the unchanged right-stick input actually points. */
        float ax=fmaxf(-1,fminf(1,dx/(60*k))),ay=fmaxf(-1,fminf(1,dy/(60*k))),an=hypotf(ax,ay);
        p[3]=d>10*k && an>0?ax/an:0; p[4]=d>10*k && an>0?ay/an:0; p[5]=c->active;
    }
    dm_tc_target c=aim_feedback_circle(); float *p=out+DM_ANDROID_TOUCH_CANCEL;
    p[0]=c.x; p[1]=c.y; p[2]=c.r; p[3]=aim_feedback_active();
    for(int i=0;i<DM_TC_MAX_TOUCH;++i) if(cancelled_aim[i].active) p[4]=1;
}
