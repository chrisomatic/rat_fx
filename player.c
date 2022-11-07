#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "window.h"
#include "math2d.h"
#include "gfx.h"
#include "camera.h"
#include "projectile.h"
#include "player.h"
#include "world.h"
#include "lighting.h"
#include "net.h"
#include "main.h"

float weapon_angle = 0.0;

// global vars
// ------------------------------------------------------------
Player players[MAX_CLIENTS];
Player* player = &players[0];
int player_count = 0;
uint32_t player_colors[MAX_CLIENTS] = {
    COLOR_BLUE,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_ORANGE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_PINK,
    COLOR_YELLOW,
};
int player_image_sets[PLAYER_MODELS_MAX][PLAYER_TEXTURES_MAX][PSTATE_MAX][WEAPON_TYPE_MAX];
PlayerModel player_models[PLAYER_MODELS_MAX];

Weapon weapons[WEAPON_MAX] = {0};


// ------------------------------------------------------------

static int weapon_image_sets[PLAYER_MODELS_MAX][PSTATE_MAX][WEAPON_MAX];
static float player_scale = 1.0;
static int crosshair_image;




// ------------------------------------------------------------

// static void player_update_sprite_index(Player* p);
// static void player_gun_set_position(Player* p);

// ------------------------------------------------------------



void player_init_models()
{
    int idx = HUMAN1;
    player_models[idx].index = idx;
    player_models[idx].name = "human1";
    player_models[idx].textures = 1;
    // player_model_texture_count += player_models[idx].textures;

    // idx = HUMAN2;
    // player_models[idx].index = idx;
    // player_models[idx].name = "human2";
    // player_models[idx].textures = 1;
    // player_model_texture_count += player_models[idx].textures;
}


void player_init_images()
{

    int ew = 128;
    int eh = 128;

    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int t = 0; t < PLAYER_TEXTURES_MAX; ++t)
        {
            for(int ps = 0; ps < PSTATE_MAX; ++ps)
            {
                for(int wt = 0; wt < WEAPON_TYPE_MAX; ++wt)
                {

                    // printf("%d, %d, %d, %d\n", pm, t, ps, wt);
                    char fname[100] = {0};

                    if(wt == WEAPON_TYPE_NONE)
                    {
                        sprintf(fname, "img/characters/%s_%d-%s.png", player_models[pm].name, t, player_state_str(ps));
                    }
                    else
                    {
                        sprintf(fname, "img/characters/%s_%d-%s_%s.png", player_models[pm].name, t, player_state_str(ps), weapon_type_str(wt));
                    }


                    if(access(fname, F_OK) == 0)
                    {
                        player_image_sets[pm][t][ps][wt] = gfx_load_image(fname, false, true, ew, eh, NULL);
                        printf("%s -> %d\n", fname, player_image_sets[pm][t][ps][wt]);
                    }
                }
            }

        }
    }


    crosshair_image = gfx_load_image("img/crosshair2.png", false, false, 0, 0, NULL);

    //TODO
    int img = player_get_image_index(player_models[0].index, 0, PSTATE_IDLE, WEAPON_TYPE_NONE);

    float h = gfx_images[img].visible_rects[0].h;
    player_scale = (float)PLAYER_HEIGHT/h;
}

void player_init_controls(Player* p)
{
    window_controls_clear_keys();

    // map keys
    window_controls_add_key(&p->keys, GLFW_KEY_W, PLAYER_ACTION_UP);
    window_controls_add_key(&p->keys, GLFW_KEY_S, PLAYER_ACTION_DOWN);
    window_controls_add_key(&p->keys, GLFW_KEY_A, PLAYER_ACTION_LEFT);
    window_controls_add_key(&p->keys, GLFW_KEY_D, PLAYER_ACTION_RIGHT);
    window_controls_add_key(&p->keys, GLFW_KEY_LEFT_SHIFT, PLAYER_ACTION_RUN);
    window_controls_add_key(&p->keys, GLFW_KEY_SPACE, PLAYER_ACTION_JUMP);
    window_controls_add_key(&p->keys, GLFW_KEY_E, PLAYER_ACTION_INTERACT);
    window_controls_add_key(&p->keys, GLFW_KEY_TAB, PLAYER_ACTION_TOGGLE_EQUIP_WEAPON);
    window_controls_add_key(&p->keys, GLFW_KEY_F2, PLAYER_ACTION_TOGGLE_DEBUG);
    window_controls_add_key(&p->keys, GLFW_KEY_G, PLAYER_ACTION_TOGGLE_GUN);

    window_controls_add_mouse_button(&p->keys, GLFW_MOUSE_BUTTON_LEFT, PLAYER_ACTION_PRIMARY_ACTION);
    window_controls_add_mouse_button(&p->keys, GLFW_MOUSE_BUTTON_RIGHT, PLAYER_ACTION_SECONDARY_ACTION);
}

static void player_init(int index)
{
    Player* p = &players[index];

    p->index = index;
    if(STR_EMPTY(p->name))
    {
        sprintf(p->name, "Player %d", p->index);
    }

    p->model_index = HUMAN1;
    p->model_texture = 0;
    p->state = PSTATE_IDLE;
    p->moving = false;
    p->attacking = false;
    p->weapon_ready = false;
    p->weapon = NULL;
    player_set_weapon(p, WEAPON_NONE);

    p->sprite_index = 0;
    p->sprite_index_direction = 0;

    p->phys.pos.x = 400.0;
    p->phys.pos.y = 1000.0;

    p->phys.vel.x = 0.0;
    p->phys.vel.y = 0.0;

    p->speed = 32.0;
    p->max_base_speed = 128.0;
    p->phys.max_linear_vel = p->max_base_speed;
    p->scale = player_scale;
    p->predicted_state_index = 0;


    // animation
    p->anim.curr_frame = 0;
    p->anim.max_frames = 16;
    p->anim.curr_frame_time = 0.0f;
    p->anim.max_frame_time = 0.04f;
    p->anim.finite = false;
    p->anim.curr_loop = 0;
    p->anim.max_loops = 0;
    p->anim.frame_sequence[0] = 12;
    p->anim.frame_sequence[1] = 13;
    p->anim.frame_sequence[2] = 14;
    p->anim.frame_sequence[3] = 15;
    p->anim.frame_sequence[4] = 0;
    p->anim.frame_sequence[5] = 1;
    p->anim.frame_sequence[6] = 2;
    p->anim.frame_sequence[7] = 3;
    p->anim.frame_sequence[8] = 4;
    p->anim.frame_sequence[9] = 5;
    p->anim.frame_sequence[10] = 6;
    p->anim.frame_sequence[11] = 7;
    p->anim.frame_sequence[12] = 8;
    p->anim.frame_sequence[13] = 9;
    p->anim.frame_sequence[14] = 10;
    p->anim.frame_sequence[15] = 11;

    // light for player
    p->point_light = -1;
    if(p == player)
        p->point_light = lighting_point_light_add(p->phys.pos.x,p->phys.pos.y,1.0,1.0,1.0,1.0);

    player_update_state(p);
    player_update_image(p);

    p->angle = 0.0;
    player_update_sprite_index(p);
}

void players_init()
{
    player_init_models();
    player_init_images();

    weapons_init();

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        Player* p = &players[i];
        player_init(i);
        if(role == ROLE_LOCAL || role == ROLE_CLIENT)
        {
            if(player == p)
            {
                LOGI("My player: %d", i);
                player_init_controls(p);
                p->active = true;
            }
        }

        if(p->active)
            player_count++;
    }
}

const char* player_state_str(PlayerState pstate)
{
    switch(pstate)
    {
        case PSTATE_IDLE: return "idle";
        case PSTATE_WALK: return "walk";
        case PSTATE_ATTACK1: return "attack1";
        case PSTATE_NONE: return "";
        default: return "";
    }
}

int player_get_image_index(PlayerModelIndex model_index, int texture, PlayerState pstate, WeaponType wtype)
{
    // player_image_sets[model][pstate][wtype];
    return player_image_sets[model_index][texture][pstate][wtype];
}

int players_get_count()
{
    player_count = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(players[i].active)
            player_count++;
    }

    return player_count;
}

void player_set_weapon(Player* p, WeaponIndex weapon_index)
{
    if(p->weapon != NULL)
    {
        if(p->weapon->index == weapon_index)
            return;
    }

    Weapon* w = &weapons[weapon_index];
    p->weapon = w;

    p->lmouse.cooldown = 0.0;
    p->lmouse.trigger_on_held = true;
    p->lmouse.trigger_on_press = true;
    p->lmouse.trigger_on_release = false;
    p->rmouse.cooldown = 0.0;
    p->rmouse.trigger_on_held = true;
    p->rmouse.trigger_on_press = true;
    p->rmouse.trigger_on_release = false;


    // left mouse
    if(w->primary_attack == ATTACK_SHOOT)
    {
        p->lmouse.period = w->gun.fire_period;
    }
    else if(w->primary_attack == ATTACK_MELEE)
    {
        p->lmouse.period = w->melee.period;
    }
    else if(w->primary_attack == ATTACK_POWER_MELEE)
    {
        p->lmouse.period = w->melee.period;
    }

    // right mouse
    if(w->secondary_attack == ATTACK_SHOOT)
    {
        p->rmouse.period = w->gun.fire_period;

    }
    else if(w->secondary_attack == ATTACK_MELEE)
    {
        p->rmouse.period = w->melee.period;
    }
    else if(w->secondary_attack == ATTACK_POWER_MELEE)
    {
        p->rmouse.period = w->melee.period;
    }


}

void player_update_mouse_click(bool active, bool toggled, MouseClick* mouse, float delta_t)
{
    if(mouse->cooldown > 0.0)
    {
        mouse->cooldown -= (delta_t*1000);
    }

    bool ready = mouse->cooldown <= 0.0;

    // clear the trigger
    mouse->trigger = false;

    // freshly clicked
    if(active && toggled)
    {
        // printf("lclick (%.2f)\n", mouse->cooldown);
        if(ready && mouse->trigger_on_press)
        {
            mouse->trigger = true;
        }
        mouse->held = false;

    }
    // freshly released
    else if(!active && toggled)
    {
        if(mouse->trigger_on_release)
        {
            mouse->trigger = true;
        }
        mouse->held = false;

    }
    // held
    else if(active)
    {
        if(ready && mouse->trigger_on_held)
        {
            mouse->trigger = true;
            mouse->held = true;
        }
    }

    if(ready && mouse->trigger)
    {
        mouse->cooldown = mouse->period;
    }
}


void player_update_anim_timing(Player* p)
{
    switch(p->state)
    {
        case PSTATE_IDLE:
            p->anim.max_frame_time = 0.15f;
            break;
        case PSTATE_WALK:
        {
            p->anim.max_frame_time = 0.04f;
            float pvx = player->phys.vel.x;
            float pvy = player->phys.vel.y;
            float pv = sqrt(SQ(pvx) + SQ(pvy));
            float scale = 128.0/pv;
            p->anim.max_frame_time *= scale;
        } break;
        case PSTATE_ATTACK1:
            p->anim.max_frame_time = 0.02f;
            break;
        default:
            p->anim.max_frame_time = 0.04f;
            break;
    }

}

void player_update_state(Player* p)
{
    PlayerState prior = p->state;

    if(p->attacking)
    {
        p->state = p->attacking_state;
        //TEMP
        p->anim.frame_sequence[0] = 0;
        p->anim.frame_sequence[1] = 1;
        p->anim.frame_sequence[2] = 2;
        p->anim.frame_sequence[3] = 3;
        p->anim.frame_sequence[4] = 4;
        p->anim.frame_sequence[5] = 5;
        p->anim.frame_sequence[6] = 6;
        p->anim.frame_sequence[7] = 7;
        p->anim.frame_sequence[8] = 8;
        p->anim.frame_sequence[9] = 9;
        p->anim.frame_sequence[10] = 10;
        p->anim.frame_sequence[11] = 11;
        p->anim.frame_sequence[12] = 12;
        p->anim.frame_sequence[13] = 13;
        p->anim.frame_sequence[14] = 14;
        p->anim.frame_sequence[15] = 15;
    }
    else if(p->moving)
    {
        p->state = PSTATE_WALK;
    }
    else
    {
        p->state = PSTATE_IDLE;
    }

    // reset the animation
    if(p->state != prior)
    {
        // printf("Player state change: %d -> %d\n", prior, p->state);
        p->anim.curr_frame = 0;
        p->anim.curr_frame_time = 0.0;
        p->anim.curr_loop = 0;
    }

    return;
}

void player_update_image(Player* p)
{
    if(p->weapon_ready && p->weapon->index != WEAPON_NONE)
    {
        p->image = player_get_image_index(p->model_index, p->model_texture, p->state, p->weapon->type);
    }
    else
    {
        p->image = player_get_image_index(p->model_index, p->model_texture, p->state, WEAPON_TYPE_NONE);
    }
    return;
}

void player_update_boxes(Player* p)
{
    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];

    p->phys.pos.w = vr->w*p->scale;
    p->phys.pos.h = vr->h*p->scale;

    p->hit_box.x = p->phys.pos.x;
    p->hit_box.y = p->phys.pos.y;
    p->hit_box.w = p->phys.pos.w;
    p->hit_box.h = p->phys.pos.h;

    // if(p->image == 3 && p->sprite_index == 0)
    // {
    //     // print_rect(&p->hit_box);
    //     print_rect(vr);
    // }
}


void player_update_sprite_index(Player* p)
{
    p->angle = calc_angle_rad(p->phys.pos.x, p->phys.pos.y, p->mouse_x, p->mouse_y);

    float angle_deg = DEG(p->angle);

    if(p->weapon_ready)
    {
        int sector = angle_sector(angle_deg, 16);

        if(sector == 15 || sector == 0)  // p->actions.right
            p->sprite_index_direction = 2;
        else if(sector == 1 || sector == 2)  // p->actions.up-p->actions.right
            p->sprite_index_direction = 3;
        else if(sector == 3 || sector == 4)  // p->actions.up
            p->sprite_index_direction = 4;
        else if(sector == 5 || sector == 6)  // p->actions.up-p->actions.left
            p->sprite_index_direction = 5;
        else if(sector == 7 || sector == 8)  // p->actions.left
            p->sprite_index_direction = 6;
        else if(sector == 9 || sector == 10)  // p->actions.down-p->actions.left
            p->sprite_index_direction = 7;
        else if(sector == 11 || sector == 12)  // p->actions.down
            p->sprite_index_direction = 0;
        else if(sector == 13 || sector == 14)  // p->actions.down-p->actions.right
            p->sprite_index_direction = 1;
    }
    else
    {
        if(p->actions.up && p->actions.left)
            p->sprite_index_direction = 5;
        else if(p->actions.up && p->actions.right)
            p->sprite_index_direction = 3;
        else if(p->actions.down && p->actions.left)
            p->sprite_index_direction = 7;
        else if(p->actions.down && p->actions.right)
            p->sprite_index_direction = 1;
        else if(p->actions.up)
            p->sprite_index_direction = 4;
        else if(p->actions.down)
            p->sprite_index_direction = 0;
        else if(p->actions.left)
            p->sprite_index_direction = 6;
        else if(p->actions.right)
            p->sprite_index_direction = 2;
    }

    p->sprite_index = p->sprite_index_direction * 16;

    int anim_frame_offset = p->anim.frame_sequence[p->anim.curr_frame];
    assert(anim_frame_offset >= 0);

    p->sprite_index += anim_frame_offset;
    p->sprite_index = MIN(p->sprite_index, gfx_images[p->image].element_count);
}


void player_update(Player* p, double delta_t)
{
    p->actions.up               = IS_BIT_SET(p->keys,PLAYER_ACTION_UP);
    p->actions.down             = IS_BIT_SET(p->keys,PLAYER_ACTION_DOWN);
    p->actions.left             = IS_BIT_SET(p->keys,PLAYER_ACTION_LEFT);
    p->actions.right            = IS_BIT_SET(p->keys,PLAYER_ACTION_RIGHT);
    p->actions.run              = IS_BIT_SET(p->keys,PLAYER_ACTION_RUN);
    p->actions.jump             = IS_BIT_SET(p->keys,PLAYER_ACTION_JUMP);
    p->actions.interact         = IS_BIT_SET(p->keys,PLAYER_ACTION_INTERACT);
    p->actions.primary_action   = IS_BIT_SET(p->keys,PLAYER_ACTION_PRIMARY_ACTION);
    p->actions.secondary_action = IS_BIT_SET(p->keys,PLAYER_ACTION_SECONDARY_ACTION);
    p->actions.toggle_equip_weapon= IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_EQUIP_WEAPON);
    p->actions.toggle_debug     = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_DEBUG);
    p->actions.toggle_gun       = IS_BIT_SET(p->keys,PLAYER_ACTION_TOGGLE_GUN);

    bool run_toggled = p->actions.run && !p->actions_prior.run;
    bool primary_action_toggled = p->actions.primary_action && !p->actions_prior.primary_action;
    bool secondary_action_toggled = p->actions.secondary_action && !p->actions_prior.secondary_action;
    bool equip_weapon_toggled = p->actions.toggle_equip_weapon && !p->actions_prior.toggle_equip_weapon;
    bool debug_toggled = p->actions.toggle_debug && !p->actions_prior.toggle_debug;
    bool gun_toggled = p->actions.toggle_gun && !p->actions_prior.toggle_gun;

    memcpy(&p->actions_prior, &p->actions, sizeof(PlayerActions));

    player_update_mouse_click(p->actions.primary_action, primary_action_toggled, &p->lmouse, delta_t);
    player_update_mouse_click(p->actions.secondary_action, secondary_action_toggled, &p->rmouse, delta_t);


    if(p->weapon != NULL)
    {
        if(p->weapon_ready || p->weapon->index == WEAPON_NONE)
        {
            if(p->lmouse.trigger)
            {

                if(p->weapon->primary_attack == ATTACK_SHOOT)
                {
                    // spawn projectile
                    weapon_fire(p->mouse_x, p->mouse_y, p->weapon, p->lmouse.held);

                }
                else if(p->weapon->primary_attack == ATTACK_MELEE)
                {
                    p->attacking = true;
                    p->attacking_state = p->weapon->primary_state;
                }
                else if(p->weapon->primary_attack == ATTACK_POWER_MELEE)
                {
                    p->attacking = true;
                    p->attacking_state = p->weapon->primary_state;
                }

            }

            if(p->rmouse.trigger)
            {
                if(p->weapon->secondary_attack == ATTACK_SHOOT)
                {
                    // spawn projectile
                    weapon_fire(p->mouse_x, p->mouse_y, p->weapon, p->rmouse.held);

                }
                else if(p->weapon->secondary_attack == ATTACK_MELEE)
                {
                    p->attacking = true;
                    p->attacking_state = p->weapon->secondary_state;
                }
                else if(p->weapon->secondary_attack == ATTACK_POWER_MELEE)
                {
                    p->attacking = true;
                    p->attacking_state = p->weapon->secondary_state;
                }
            }
        }
    }


    if(gun_toggled)
    {
        int next = p->weapon->index+1;
        if(next >= WEAPON_MAX) next = 0;
        player_set_weapon(p, next);
    }

    if(equip_weapon_toggled)
    {
        p->weapon_ready = !p->weapon_ready;
    }

    if(debug_toggled)
    {
        debug_enabled = !debug_enabled;
    }

    if(role != ROLE_SERVER)
    {
        if(p->actions.primary_action)
        {
            if(window_is_cursor_enabled())
            {
                window_disable_cursor();
            }
        }
    }

    Vector2f accel = {0};
    bool moving_player = MOVING_PLAYER(p);

    if(p->actions.up)    { accel.y -= p->speed; }
    if(p->actions.down)  { accel.y += p->speed; }
    if(p->actions.left)  { accel.x -= p->speed; }
    if(p->actions.right) { accel.x += p->speed; }

    if(run_toggled)
    {
        p->running = !p->running;
    }

    p->phys.max_linear_vel = p->max_base_speed;

    if(p->running && moving_player)
    {
        accel.x *= 10.0;
        accel.y *= 10.0;
        p->phys.max_linear_vel *= 10.0;
    }

    // //TODO: weapon
    // if(p->attacking)
    // {
    //     accel.x = 0;
    //     accel.y = 0;
    // }



#if 0
    if(role == ROLE_SERVER)
    {
        printf("player pos: %f %f, accel: %f %f, delta_t: %f,map rect: %f %f %f %f",
                p->phys.pos.x,
                p->phys.pos.y,
                accel.x,
                accel.y,
                delta_t,
                map.rect.x,
                map.rect.y,
                map.rect.w,
                map.rect.h);
    }
#endif

    physics_begin(&p->phys);
    physics_add_friction(&p->phys, 16.0);
    physics_add_force(&p->phys, accel.x, accel.y);
    physics_simulate(&p->phys, delta_t);


    // // TODO: won't work for idle state
    // if(FEQ(accel.x,0.0) && FEQ(accel.y,0.0))
    // {
    //     p->moving = false;
    //     p->anim.curr_frame = 0;
    //     p->anim.curr_frame_time = 0.0;
    // }
    // else
    // {
    //     p->moving = true;
    //     gfx_anim_update(&p->anim,delta_t);
    // }


    // is this bueno?
    p->moving = !(FEQ(accel.x,0.0) && FEQ(accel.y,0.0));


    player_update_state(p);
    player_update_anim_timing(p);
    player_update_image(p);

    gfx_anim_update(&p->anim,delta_t);

    if(p->attacking && p->anim.curr_loop > 0)
    {
        p->attacking = false;
        player_update_state(p);
        player_update_image(p);
    }

    player_update_sprite_index(p);
    player_update_boxes(p);

    limit_pos(&map.rect, &p->phys.pos);

    lighting_point_light_move(p->point_light,p->phys.pos.x, p->phys.pos.y);
}

void player_handle_net_inputs(Player* p, double delta_t)
{
    // handle input
    memcpy(&p->input_prior, &p->input, sizeof(NetPlayerInput));

    p->input.delta_t = delta_t;
    p->input.keys = p->keys;
    p->input.mouse_x = p->mouse_x;
    p->input.mouse_y = p->mouse_y;
    
    if(p->input.keys != p->input_prior.keys || p->input.mouse_x != p->input_prior.mouse_x || p->input.mouse_y != p->input_prior.mouse_y)
    {
        net_client_add_player_input(&p->input);

        if(net_client_get_input_count() >= 3) // @HARDCODED 3
        {
            // add position, angle to predicted player state
            NetPlayerState* state = &p->predicted_states[p->predicted_state_index];

            // circular buffer
            if(p->predicted_state_index == MAX_CLIENT_PREDICTED_STATES -1)
            {
                // shift
                for(int i = 1; i <= MAX_CLIENT_PREDICTED_STATES -1; ++i)
                {
                    memcpy(&p->predicted_states[i-1],&p->predicted_states[i],sizeof(NetPlayerState));
                }
            }
            else if(p->predicted_state_index < MAX_CLIENT_PREDICTED_STATES -1)
            {
                p->predicted_state_index++;
            }

            state->associated_packet_id = net_client_get_latest_local_packet_id();
            state->pos.x = p->phys.pos.x;
            state->pos.y = p->phys.pos.y;
            state->angle = p->angle;
        }
    }
}

//TODO: fix this function
void player_update_other(Player* p, double delta_t)
{
    p->lerp_t += delta_t;

    float tick_time = 1.0/TICK_RATE;
    float t = (p->lerp_t / tick_time);

    Vector2f lp = lerp2f(&p->state_prior.pos,&p->state_target.pos,t);
    p->phys.pos.x = lp.x;
    p->phys.pos.y = lp.y;

    p->angle = lerp(p->state_prior.angle,p->state_target.angle,t);

    // player_gun_set_position(p);
}

void player_draw(Player* p)
{
    if(!is_in_camera_view(&p->phys.pos))
    {
        return;
    }

    GFXImage* img = &gfx_images[p->image];
    Rect* vr = &img->visible_rects[p->sprite_index];
    // float angle_deg = DEG(p->angle);

    // offset for drawing sprites
    float img_center_x = img->element_width/2.0;
    float img_center_y = img->element_height/2.0;
    float offset_x = -(vr->x - img_center_x)*p->scale;
    float offset_y = -(vr->y - img_center_y)*p->scale;
    float px = p->phys.pos.x + offset_x;
    float py = p->phys.pos.y + offset_y;

    // player
    gfx_draw_image(p->image, p->sprite_index, px, py, COLOR_TINT_NONE,p->scale,0.0,1.0,true);

    bool draw_weapon = p->weapon_ready && p->weapon->index != WEAPON_NONE;

    if(draw_weapon)
    {

#if 1
        int wimage = weapons_get_image_index(p->model_index, p->state, p->weapon->type);
        GFXImage* wimg = &gfx_images[wimage];
        Rect* wvr = &wimg->visible_rects[p->sprite_index];

        p->weapon->pos.x = p->phys.pos.x + (wvr->x-vr->x)*p->scale;
        p->weapon->pos.y = p->phys.pos.y + (wvr->y-vr->y)*p->scale;

        // weapon
        gfx_draw_image(wimage, p->sprite_index, px, py, COLOR_TINT_NONE,p->scale,0,1.0,true);

#else


        int wimage = weapons_get_image_index(p->model_index, p->state, p->weapon->type);
        GFXImage* wimg = &gfx_images[wimage];
        Rect* wvr = &wimg->visible_rects[p->sprite_index];

        float wimg_center_x = wimg->element_width/2.0;
        float wimg_center_y = wimg->element_height/2.0;

        float gx = p->phys.pos.x + (wvr->x-wimg_center_x)*p->scale + offset_x;
        float gy = p->phys.pos.y + (wvr->y-wimg_center_y)*p->scale + offset_y;

        p->weapon->pos.x = gx;
        p->weapon->pos.y = gy;


        // float angle_deg = DEG(p->angle);
        float angle_deg = calc_angle_deg(p->weapon->pos.x, p->weapon->pos.y, p->mouse_x, p->mouse_y);
        int sector = angle_sector(angle_deg, 16);
        // int sector = angle_sector(angle_deg, 16);
        int sector2 = angle_sector(angle_deg, 8);
        float sector_center = 0.0;


        if(sector == 0)
            sector_center = 11.25*2;
        else if(sector == 15)
            sector_center = 360.0-11.25*2;
        else if(sector == 1 || sector == 2)
            sector_center = 45.0;
        else if(sector == 3 || sector == 4)
            sector_center = 90.0;
        else if(sector == 5 || sector == 6)
            sector_center = 135.0;
        else if(sector == 7 || sector == 8)
            sector_center = 180.0;
        else if(sector == 9 || sector == 10)
            sector_center = 225.0;
        else if(sector == 11 || sector == 12)
            sector_center = 270.0;
        else if(sector == 13 || sector == 14)
            sector_center = 315.0;


        float angle_rotate = (angle_deg-sector_center)/2.0;
        weapon_angle = angle_rotate;
        printf("%.2f (%d, %d),  %.2f,   %.2f\n", angle_deg, sector, sector2, sector_center, angle_rotate);

        // weapon
        gfx_draw_image(wimage, p->sprite_index, p->weapon->pos.x, p->weapon->pos.y, COLOR_TINT_NONE,p->scale,angle_rotate,1.0,false);
#endif
    }




    if(debug_enabled)
    {
        gfx_draw_rect(&p->hit_box, COLOR_RED, 1.0,1.0, false, true);

        // if(draw_weapon)
        // {
        //     Rect rw = {0};
        //     rw.x = p->weapon->pos.x;
        //     rw.y = p->weapon->pos.y;
        //     rw.w = 2;
        //     rw.h = 2;
        //     gfx_draw_rect(&rw, COLOR_ORANGE, 1.0,1.0, true, true);
        // }

        // Rect r = {0};
        // r.x = p->phys.pos.x;
        // r.y = p->phys.pos.y;
        // r.w = 2;
        // r.h = 2;
        // gfx_draw_rect(&r, COLOR_PURPLE, 1.0,1.0, true, true);
    }

    gfx_draw_image(crosshair_image,0,p->mouse_x,p->mouse_y, COLOR_PURPLE,1.0,0.0,0.80, false);

    Vector2f size = gfx_string_get_size(0.1, p->name);
    gfx_draw_string(p->phys.pos.x - size.x/2.0, p->phys.pos.y + p->phys.pos.h/2.0,player_colors[p->index],0.1,0.0, 0.8, true, true, p->name);
}



void weapons_init()
{

    int idx = WEAPON_PISTOL1;
    weapons[idx].index = idx;
    weapons[idx].name = "pistol1";
    weapons[idx].type = WEAPON_TYPE_HANDGUN;

    weapons[idx].primary_attack = ATTACK_SHOOT;
    weapons[idx].primary_state = PSTATE_NONE;    // no change in state

    weapons[idx].secondary_attack = ATTACK_MELEE;
    weapons[idx].secondary_state = PSTATE_ATTACK1;

    weapons[idx].gun.power = 1.0;
    weapons[idx].gun.recoil_spread = 2.0;
    weapons[idx].gun.fire_range = 500.0;
    weapons[idx].gun.fire_speed = 1000.0;
    weapons[idx].gun.fire_period = 500.0; // milliseconds
    weapons[idx].gun.fire_spread = 0.0;
    weapons[idx].gun.fire_count = 1;
    weapons[idx].gun.bullets = 100;
    weapons[idx].gun.bullets_max = 100;
    weapons[idx].gun.projectile_type = PROJECTILE_TYPE_BULLET;

    weapons[idx].melee.range = 8.0;
    weapons[idx].melee.power = 0.1;
    weapons[idx].melee.period = 1000.0;


    idx = WEAPON_MACHINEGUN1;
    weapons[idx].index = idx;
    weapons[idx].name = "pistol1";
    weapons[idx].type = WEAPON_TYPE_HANDGUN;

    weapons[idx].primary_attack = ATTACK_SHOOT;
    weapons[idx].primary_state = PSTATE_NONE;    // no change in state

    weapons[idx].secondary_attack = ATTACK_MELEE;
    weapons[idx].secondary_state = PSTATE_ATTACK1;

    weapons[idx].gun.power = 1.0;
    weapons[idx].gun.recoil_spread = 4.0;
    weapons[idx].gun.fire_range = 500.0;
    weapons[idx].gun.fire_speed = 1000.0;
    weapons[idx].gun.fire_period = 100.0; // milliseconds
    weapons[idx].gun.fire_spread = 0.0;
    weapons[idx].gun.fire_count = 1;
    weapons[idx].gun.bullets = 100;
    weapons[idx].gun.bullets_max = 100;
    weapons[idx].gun.projectile_type = PROJECTILE_TYPE_BULLET;

    weapons[idx].melee.range = 8.0;
    weapons[idx].melee.power = 0.1;
    weapons[idx].melee.period = 1000.0;



    idx = WEAPON_SHOTGUN1;
    weapons[idx].index = idx;
    weapons[idx].name = "pistol1";
    weapons[idx].type = WEAPON_TYPE_HANDGUN;

    weapons[idx].primary_attack = ATTACK_SHOOT;
    weapons[idx].primary_state = PSTATE_NONE;    // no change in state

    weapons[idx].secondary_attack = ATTACK_MELEE;
    weapons[idx].secondary_state = PSTATE_ATTACK1;

    weapons[idx].gun.power = 1.0;
    weapons[idx].gun.recoil_spread = 0.0;
    weapons[idx].gun.fire_range = 200.0;
    weapons[idx].gun.fire_speed = 1000.0;
    weapons[idx].gun.fire_period = 400.0; // milliseconds
    weapons[idx].gun.fire_spread = 30.0;
    weapons[idx].gun.fire_count = 5;
    weapons[idx].gun.bullets = 100;
    weapons[idx].gun.bullets_max = 100;
    weapons[idx].gun.projectile_type = PROJECTILE_TYPE_BULLET;

    weapons[idx].melee.range = 8.0;
    weapons[idx].melee.power = 0.1;
    weapons[idx].melee.period = 1000.0;



    idx = WEAPON_NONE;
    weapons[idx].index = idx;
    weapons[idx].name = "";
    weapons[idx].type = WEAPON_TYPE_MELEE;

    weapons[idx].primary_attack = ATTACK_MELEE;
    weapons[idx].primary_state = PSTATE_ATTACK1;

    weapons[idx].secondary_attack = ATTACK_POWER_MELEE;
    weapons[idx].secondary_state = PSTATE_ATTACK1;

    weapons[idx].melee.range = 8.0;
    weapons[idx].melee.power = 0.1;
    weapons[idx].melee.period = 100.0;

    weapons_init_images();
}

// must call this after players_init()
void weapons_init_images()
{
    int ew = 128;
    int eh = 128;

    for(int pm = 0; pm < PLAYER_MODELS_MAX; ++pm)
    {
        for(int ps = 0; ps < PSTATE_MAX; ++ps)
        {
            for(int w = 0; w < WEAPON_MAX; ++w)
            {
                if(w == WEAPON_NONE)
                {
                    weapon_image_sets[pm][ps][w] = weapon_image_sets[pm][ps][WEAPON_PISTOL1];
                }
                else
                {
                    char fname[100] = {0};
                    sprintf(fname, "img/characters/%s-%s_%s_%s.png", player_models[pm].name, player_state_str(ps), weapon_type_str(weapons[w].type), weapons[w].name);
                    weapon_image_sets[pm][ps][w] = gfx_load_image(fname, false, false, ew, eh, NULL);
                }
            }
        }

    }
}


// int weapons_get_image_index(PlayerModelIndex model_index, PlayerState pstate, WeaponType wtype)
int weapons_get_image_index(PlayerModelIndex model_index, PlayerState pstate, WeaponType wtype)
{
    return weapon_image_sets[model_index][pstate][wtype];
}

const char* weapon_type_str(WeaponType wtype)
{
    switch(wtype)
    {
        case WEAPON_TYPE_HANDGUN: return "handgun";
        case WEAPON_TYPE_MELEE: return "melee";
        default: return "";
    }
}



void weapon_fire(int mx, int my, Weapon* weapon, bool held)
{
    if(weapon->gun.fire_count > 1)
    {
        for(int i = 0; i < weapon->gun.fire_count; ++i)
        {
            int direction = rand()%2 == 0 ? -1 : 1;
            float angle_offset = rand_float_between(0.0, weapon->gun.fire_spread/2.0) * direction;
            projectile_add(weapon->gun.projectile_type, weapon, mx, my, angle_offset);
        }
    }
    else
    {
        float angle_offset = 0.0;
        if(held && !FEQ(weapon->gun.recoil_spread,0.0))
        {
            int direction = rand()%2 == 0 ? -1 : 1;
            angle_offset = rand_float_between(0.0, weapon->gun.recoil_spread/2.0) * direction;
        }
        projectile_add(weapon->gun.projectile_type, weapon, mx, my, angle_offset);
    }
}
