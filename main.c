#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

#define BULLET_SPEED 1000.0f
#define FIRE_RATE 10.0f
#define FIRE_COOLDOWN (1.0f / FIRE_RATE)

#define SHOOT_VOICES 10

typedef struct {
  Vector2 pos;
  Vector2 vel;
  Vector2 acc;
  float size;
  float rotation;
  Texture2D tex;
  float tex_scale;
} Ship;

typedef struct {
  Vector2 pos;
  Vector2 vel;
  Vector2 size;
  float rotation;
  bool active;
  Texture2D *tex;
} Bullet;

typedef struct {
  Vector2 pos;
  float radius;
  int num_points;
  float points[100];
  float cooldowns[100];
  float rotation_speed;
  float current_rotation;
} Emitter;

typedef struct {
  Vector2 pos;
  Vector2 vel;
  float radius;
  bool active;
} Emitter_Bullet;

static float clampf(float v, float a, float b) {
  return (v < a) ? a : (v > b) ? b : v;
}

void draw_emitter_bullets(Emitter *emitter, Emitter_Bullet *emitter_bullets,
                          int max_bullets) {
  for (int i = 0; i < max_bullets; i++) {
    if (emitter_bullets[i].active) {
      DrawCircleV(emitter_bullets[i].pos, emitter_bullets[i].radius,
                  (Color){0xFF, 0xA5, 0x10, 0xFF});
    }
  }

  DrawCircleV(emitter->pos, emitter->radius, (Color){0x18, 0x18, 0x18, 0xFF});
}

void update_emitter_bullets(Emitter *emitter, Emitter_Bullet *emitter_bullets,
                            int sw, int sh, int max_bullets, float dt) {
  emitter->current_rotation += emitter->rotation_speed * dt;
  float current_rot_rad = emitter->current_rotation * DEG2RAD;

  float angle_step = (2.0f * PI) / emitter->num_points;

  for (int i = 0; i < emitter->num_points; i++) {
    emitter->cooldowns[i] -= dt;

    if (emitter->cooldowns[i] <= 0.0f) {
      float point_angle = current_rot_rad + (i * angle_step);
      Vector2 dir = {cosf(point_angle), sinf(point_angle)};

      Vector2 spawn_pos = {emitter->pos.x + dir.x * emitter->radius,
                           emitter->pos.y + dir.y * emitter->radius};

      for (int j = 0; j < max_bullets; j++) {
        if (!emitter_bullets[j].active) {
          emitter_bullets[j].active = true;
          emitter_bullets[j].pos = spawn_pos;

          emitter_bullets[j].vel = (Vector2){dir.x * 200.0f, dir.y * 200.0f};
          emitter_bullets[j].radius = 24.0f;

          emitter->cooldowns[i] = 1.0f / emitter->points[i];
          break;
        }
      }
    }
  }

  for (int i = 0; i < max_bullets; i++) {
    if (emitter_bullets[i].active) {
      emitter_bullets[i].pos.x += emitter_bullets[i].vel.x * dt;
      emitter_bullets[i].pos.y += emitter_bullets[i].vel.y * dt;

      if (emitter_bullets[i].pos.x < -100 ||
          emitter_bullets[i].pos.x > sw + 100 ||
          emitter_bullets[i].pos.y < -100 ||
          emitter_bullets[i].pos.y > sh + 100) {
        emitter_bullets[i].active = false;
      }
    }
  }
}

void draw_ship(Ship *ship, Texture2D shadow_tex) {
  Texture2D ship_tex = ship->tex;

  float w = ship_tex.width * ship->tex_scale;
  float h = ship_tex.height * ship->tex_scale;

  Rectangle ship_src = {0, 0, (float)ship_tex.width, (float)ship_tex.height};
  Rectangle ship_dst = {ship->pos.x, ship->pos.y, w, h};
  Vector2 ship_origin = {w * 0.5f, h * 0.5f};

  float angleDeg = ship->rotation * RAD2DEG;

  Vector2 light_pos = (Vector2){120.0f, 80.0f};

  Vector2 d = Vector2Subtract(ship->pos, light_pos);
  float len = sqrtf(d.x * d.x + d.y * d.y);
  if (len > 0.0001f) {
    d.x /= len;
    d.y /= len;
  } else {
    d = (Vector2){1.0f, 1.0f};
  }

  float dist01 = clampf(len / 900.0f, 0.0f, 1.0f);

  float offset_px = 5.0f + 6.0f * dist01;
  Vector2 shadow_offset = (Vector2){d.x * offset_px, d.y * offset_px};

  float shadow_scale = 1.00f;

  float sw = w * shadow_scale;
  float sh = h * shadow_scale;

  Rectangle sh_src = {0, 0, (float)shadow_tex.width, (float)shadow_tex.height};
  Rectangle sh_dst_base = {ship->pos.x + shadow_offset.x,
                           ship->pos.y + shadow_offset.y, sw, sh};
  Vector2 sh_origin = {sw * 0.5f, sh * 0.5f};

  const Vector2 taps[] = {
      {0, 0}, {1, 0},  {-1, 0}, {0, 1},   {0, -1},
      {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
  };

  DrawTexturePro(shadow_tex, sh_src, sh_dst_base, sh_origin, angleDeg, WHITE);
  DrawTexturePro(ship_tex, ship_src, ship_dst, ship_origin, angleDeg, WHITE);
}

void update_ship(Ship *ship, int sw, int sh, float dt) {
  if (IsKeyDown(KEY_A))
    ship->rotation -= 4.0f * dt;
  if (IsKeyDown(KEY_D))
    ship->rotation += 4.0f * dt;

  float thrust = 450.0f;
  if (IsKeyDown(KEY_W)) {
    ship->vel.x += sinf(ship->rotation) * thrust * dt;
    ship->vel.y += -cosf(ship->rotation) * thrust * dt;
  }
  if (IsKeyDown(KEY_S)) {
    ship->vel.x -= sinf(ship->rotation) * thrust * dt;
    ship->vel.y -= -cosf(ship->rotation) * thrust * dt;
  }

  float damping = 0.99f;
  ship->vel.x *= powf(damping, dt * 60.0f);
  ship->vel.y *= powf(damping, dt * 60.0f);

  ship->pos.x += ship->vel.x * dt;
  ship->pos.y += ship->vel.y * dt;

  float r = ship->size * 0.5f;
  ship->pos.x = clampf(ship->pos.x, r, sw - r);
  ship->pos.y = clampf(ship->pos.y, r, sh - r);
  if (ship->pos.x <= r || ship->pos.x >= sw - r)
    ship->vel.x = 0;
  if (ship->pos.y <= r || ship->pos.y >= sh - r)
    ship->vel.y = 0;
}

void update_bullets(Bullet *bullets, int max_bullets, float dt) {
  for (int i = 0; i < max_bullets; i++) {
    Bullet *b = &bullets[i];
    if (!b->active)
      continue;

    b->pos.x += b->vel.x * dt;
    b->pos.y += b->vel.y * dt;
  }
}

void draw_bullet(Bullet *b) {
  if (!b->tex)
    return;

  Texture2D tex = *b->tex;

  Rectangle src = {0, 0, (float)tex.width, (float)tex.height};

  Rectangle dst = {b->pos.x, b->pos.y, b->size.x, b->size.y};

  Vector2 origin = {b->size.x * 0.5f, b->size.y * 0.5f};

  DrawTexturePro(tex, src, dst, origin, b->rotation * RAD2DEG, WHITE);
}

void calculate_ship_pos(Ship *ship) {
  ship->vel.x += ship->acc.x;
  ship->vel.y += ship->acc.y;

  ship->pos.x += ship->vel.x;
  ship->pos.y += ship->vel.y;
}

int main(void) {
  int monitor = GetCurrentMonitor();
  int screenW = GetMonitorWidth(monitor);
  int screenH = GetMonitorHeight(monitor);

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(screenW, screenH, "Nieme");
  ToggleFullscreen();
  SetTargetFPS(60);
  screenW = GetMonitorWidth(monitor);
  screenH = GetMonitorHeight(monitor);
  SetTargetFPS(120);
  InitAudioDevice();

  Sound shoot_base = LoadSound("assets/sfx/shoot.mp3");
  SetSoundVolume(shoot_base, 0.5f);

  Sound shoot_voices[SHOOT_VOICES] = {0};
  for (int i = 0; i < SHOOT_VOICES; i++) {
    shoot_voices[i] = LoadSoundAlias(shoot_base);
    SetSoundVolume(shoot_voices[i], 0.5f);
  }

  int shoot_voice = 0;

  Texture2D bg_tile = LoadTexture("assets/sprites/bg_tile.png");
  Texture2D ship_texture = LoadTexture("assets/sprites/ship.png");
  Texture2D ship_shadow_texture = LoadTexture("assets/sprites/ship_shadow.png");
  Texture2D bullet_texture = LoadTexture("assets/sprites/bullet.png");

  SetTextureFilter(bg_tile, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(ship_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(ship_shadow_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(bullet_texture, TEXTURE_FILTER_TRILINEAR);

  Ship player_ship = {.pos = {(float)screenW / 2, (float)screenH / 2},
                      .acc = {0, 0},
                      .vel = {0, 0},
                      .size = 100.0f,
                      .rotation = 0,
                      .tex = ship_texture,
                      .tex_scale = 1.0f};

  player_ship.tex_scale = player_ship.size / (float)player_ship.tex.height;

  int max_bullets = 100;
  Bullet *bullets = malloc(sizeof(Bullet) * max_bullets);
  int next_bullet = 0;
  for (int i = 0; i < max_bullets; i++) {
    bullets[i].active = false;
    bullets[i].tex = &bullet_texture;
  }

  float shoot_cooldown = 0.0f;

  Emitter emitter = {.pos = {(float)screenW / 4, (float)screenH / 4},
                     .radius = 60.0f,
                     .num_points = 4,
                     .points = {2.0f, 2.0f, 2.0f, 2.0f},
                     .cooldowns = {0},
                     .rotation_speed = 30.0f,
                     .current_rotation = 0.0f};
  int max_emitter_bullets = 200;
  Emitter_Bullet *emitter_bullets =
      malloc(sizeof(Emitter_Bullet) * max_emitter_bullets);
  int next_emitter_bullet = 0;
  for (int i = 0; i < max_bullets; i++) {
    emitter_bullets[i].active = false;
  }

  while (!WindowShouldClose()) {
    int sw = GetRenderWidth();
    int sh = GetRenderHeight();
    float dt = GetFrameTime();
    if (shoot_cooldown > 0.0f)
      shoot_cooldown -= dt;

    if (IsKeyDown(KEY_SPACE) && shoot_cooldown <= 0.0f) {
      Bullet *b = &bullets[next_bullet];

      float s = sinf(player_ship.rotation);
      float c = cosf(player_ship.rotation);
      Vector2 forward = (Vector2){s, -c};

      float muzzle_offset = player_ship.size * 0.5f;
      b->pos = (Vector2){player_ship.pos.x + forward.x * muzzle_offset,
                         player_ship.pos.y + forward.y * muzzle_offset};

      b->vel = (Vector2){player_ship.vel.x + forward.x * BULLET_SPEED,
                         player_ship.vel.y + forward.y * BULLET_SPEED};

      b->size = (Vector2){40.0f, 80.0f};
      b->rotation = player_ship.rotation;
      b->active = true;

      next_bullet = (next_bullet + 1) % max_bullets;

      shoot_cooldown = FIRE_COOLDOWN;

      PlaySound(shoot_voices[shoot_voice]);
      shoot_voice = (shoot_voice + 1) % SHOOT_VOICES;
    }

    update_ship(&player_ship, sw, sh, dt);
    update_bullets(bullets, max_bullets, dt);
    update_emitter_bullets(&emitter, emitter_bullets, sw, sh,
                           max_emitter_bullets, dt);

    BeginDrawing();
    float bg_scale = 0.25f;

    int tile_w = (int)(bg_tile.width * bg_scale);
    int tile_h = (int)(bg_tile.height * bg_scale);

    Rectangle src = {0, 0, (float)bg_tile.width, (float)bg_tile.height};

    for (int y = 0; y < sh; y += tile_h) {
      for (int x = 0; x < sw; x += tile_w) {
        Rectangle dst = {(float)x, (float)y, (float)tile_w, (float)tile_h};
        DrawTexturePro(bg_tile, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
      }
    }

    for (int i = 0; i < max_bullets; i++) {
      if (bullets[i].active)
        draw_bullet(&bullets[i]);
    }
    draw_ship(&player_ship, ship_shadow_texture);
    draw_emitter_bullets(&emitter, emitter_bullets, max_emitter_bullets);
    EndDrawing();
  }

  UnloadTexture(bullet_texture);
  UnloadTexture(ship_texture);
  UnloadTexture(ship_shadow_texture);
  UnloadTexture(bg_tile);
  for (int i = 0; i < SHOOT_VOICES; i++)
    UnloadSoundAlias(shoot_voices[i]);
  UnloadSound(shoot_base);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
