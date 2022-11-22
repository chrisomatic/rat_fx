#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "main.h"
#include "math2d.h"
#include "window.h"
#include "camera.h"
#include "world.h"
#include "gfx.h"
#include "log.h"
#include "player.h"
#include "zombie.h"
#include "effects.h"
#include "particles.h"
#include "projectile.h"

typedef struct
{
    ProjectileType type;
    Vector2f pos;
    Vector2f vel;
    float angle_deg;
    float power;
    int damage;
    int sprite_index;
    Rect hurt_box;
    Rect hurt_box_prior;
    float time;
    float ttl;
    bool dead;
} Projectile;


#define MAX_PROJECTILES 1024
static Projectile projectiles[MAX_PROJECTILES];
glist* plist = NULL;

static int projectile_image_set;



static void projectile_remove(int index)
{
    list_remove(plist, index);
}

static void update_hurt_box(Projectile* proj)
{
    memcpy(&proj->hurt_box_prior,&proj->hurt_box,sizeof(Rect));
    proj->hurt_box.x = proj->pos.x;
    proj->hurt_box.y = proj->pos.y;
}

void projectile_init()
{
    plist = list_create((void*)projectiles, MAX_PROJECTILES, sizeof(Projectile));
    projectile_image_set = gfx_load_image("img/projectile_set.png", false, false, 32, 32, NULL);
}

void projectile_add(Player* p, Gun* gun, float angle_offset)
{
    Projectile proj = {0};

    float speed = gun->fire_speed;
    // speed = 10.0;

    proj.sprite_index = gun->projectile_type;
    proj.damage = gun->power + proj.power;
    proj.dead = false;

    float _x = gun->pos.x;
    float _y = gun->pos.y;
    proj.pos.x = _x;
    proj.pos.y = _y;

    Rect* vr = &gfx_images[projectile_image_set].visible_rects[proj.sprite_index];
    proj.hurt_box.w = vr->w;
    proj.hurt_box.h = vr->h;
    update_hurt_box(&proj);

    int mx = p->mouse_x;
    int my = p->mouse_y;

    Rect mouse_r = {0};
    mouse_r.x = mx;
    mouse_r.y = my;
    mouse_r.w = 1;
    mouse_r.h = 1;

    float angle_deg = angle_offset;
    if(rectangles_colliding(&mouse_r, &p->pos))
    {
        angle_deg += DEG(p->angle);
    }
    else
    {
        angle_deg += calc_angle_deg(proj.pos.x, proj.pos.y, mx, my);
    }

    float angle = RAD(angle_deg);
    proj.angle_deg = angle_deg;
    proj.vel.x = speed*cosf(angle);
    proj.vel.y = speed*sinf(angle);
    proj.time = 0.0;
    float vel = sqrt(proj.vel.x*proj.vel.x + proj.vel.y*proj.vel.y);
    proj.ttl  = 1.0 / (vel / gun->fire_range);

    list_add(plist, (void*)&proj);
}


void projectile_update(float delta_t)
{
    for(int i = plist->count - 1; i >= 0; --i)
    {
        Projectile* proj = &projectiles[i];

        if(proj->dead)
            continue;

        proj->time += delta_t;
        if(proj->time >= proj->ttl)
        {
            proj->dead = true;
            continue;
        }

        proj->pos.x += delta_t*proj->vel.x;
        proj->pos.y -= delta_t*proj->vel.y; // @minus

        update_hurt_box(proj);

        for(int j = zlist->count - 1; j >= 0; --j) // num_zombies
        {

            if(proj->dead)
                break;
#if 0
            if(is_colliding(&proj->hurt_box, &zombies[j].hit_box))
            {
                projectile_remove(i);
            }
#else
            if(are_rects_colliding(&proj->hurt_box_prior, &proj->hurt_box, &zombies[j].hit_box))
            {
                //printf("Bullet collided!\n");
                proj->dead = true;

                Vector2f force = {
                    100.0*cosf(RAD(proj->angle_deg)),
                    100.0*sinf(RAD(proj->angle_deg))
                };
                //zombie_push(j,&force);

                if(role == ROLE_SERVER)
                {
                    printf("Zombie %d hurt for %d damage!\n",j,proj->damage);
                }

                particles_spawn_effect(zombies[j].phys.pos.x, zombies[j].phys.pos.y, &particle_effects[EFFECT_BLOOD1], 0.6, true, false);
                zombie_hurt(j,proj->damage);

            }
#endif
        }

        float x0 = proj->hurt_box.x;
        float y0 = proj->hurt_box.y;
        float x1 = proj->hurt_box_prior.x;
        float y1 = proj->hurt_box_prior.y;

        //printf("p0 (%f %f) -> p1 (%f %f)\n",x0,y0,x1,y1);
        gfx_add_line(x0,y0,x1,y1, 0x00FFFF00);
        gfx_add_line(x0+1,y0+1,x1+1,y1+1, 0x00555555);
    }

    for(int i = plist->count - 1; i >= 0; --i)
    {
        if(projectiles[i].dead)
        {
            projectile_remove(i);
        }
    }

}

void projectile_draw()
{
    for(int i = 0; i < plist->count; ++i)
    {
        Projectile* proj = &projectiles[i];

        if(is_in_camera_view(&proj->hurt_box))
        {
            //gfx_add_line(proj->hurt_box.x + proj->hurt_box.w/2.0, proj->hurt_box.y + proj->hurt_box.h/2.0, proj->hurt_box_prior.x + proj->hurt_box_prior.w/2.0, proj->hurt_box_prior.y + proj->hurt_box_prior.h/2.0, 0x00FFFF00);

            //gfx_draw_image(projectile_image_set,proj->sprite_index,proj->pos.x,proj->pos.y, COLOR_TINT_NONE,1.0, proj->angle_deg, 1.0, false,true);

            if(debug_enabled)
            {
                gfx_draw_rect(&proj->hurt_box, 0x0000FFFF, 0.0, 1.0,1.0, false, true);
            }
        }
    }
}
