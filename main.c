#include <math.h>
#include <raylib.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define BULLET_SPEED 500.0f
#define FIRE_RATE 10.0f
#define FIRE_COOLDOWN (1.0f / FIRE_RATE)

#define SHOOT_VOICES 10

typedef struct {
  Vector2 pos;
  Vector2 vel;
  Vector2 acc;
  float size;
  float rotation;
  Color color;
} Ship;

typedef struct {
  Vector2 pos;
  Vector2 vel;
  Vector2 size;
  float rotation;
  Color color;
  bool active;
} Bullet;

void draw_triangle(Vector2 pos, float size, float rotation, Color color) {
  Vector2 p1 = {0.0f, -size};
  Vector2 p2 = {-0.87f * size, 0.5f * size};
  Vector2 p3 = {0.87f * size, 0.5f * size};

  float s = sinf(rotation);
  float c = cosf(rotation);

  p1 = (Vector2){p1.x * c - p1.y * s, p1.x * s + p1.y * c};
  p2 = (Vector2){p2.x * c - p2.y * s, p2.x * s + p2.y * c};
  p3 = (Vector2){p3.x * c - p3.y * s, p3.x * s + p3.y * c};

  p1.x += pos.x;
  p1.y += pos.y;
  p2.x += pos.x;
  p2.y += pos.y;
  p3.x += pos.x;
  p3.y += pos.y;

  DrawTriangle(p1, p2, p3, color);
}

void draw_ship(Ship *ship) {
  Vector2 p1 = {0.87f * ship->size, -0.5f * ship->size};
  Vector2 p2 = {-0.87f * ship->size, -0.5f * ship->size};
  Vector2 p3 = {0.0f, ship->size};

  float s = sinf(ship->rotation);
  float c = cosf(ship->rotation);

  p1 = (Vector2){p1.x * c - p1.y * s, p1.x * s + p1.y * c};
  p2 = (Vector2){p2.x * c - p2.y * s, p2.x * s + p2.y * c};
  p3 = (Vector2){p3.x * c - p3.y * s, p3.x * s + p3.y * c};

  p1.x += ship->pos.x;
  p1.y += ship->pos.y;
  p2.x += ship->pos.x;
  p2.y += ship->pos.y;
  p3.x += ship->pos.x;
  p3.y += ship->pos.y;

  Vector2 mid = (Vector2){(p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f};

  Vector2 d = (Vector2){mid.x - p3.x, mid.y - p3.y};
  float len = sqrtf(d.x * d.x + d.y * d.y);

  Vector2 dir = (len > 0.00001f) ? (Vector2){d.x / len, d.y / len}
                                 : (Vector2){0.0f, -1.0f};

  float front_offset = 2.4f * ship->size;

  Vector2 p_front =
      (Vector2){mid.x + dir.x * front_offset, mid.y + dir.y * front_offset};

  float forward_offset = 1.2f * ship->size;
  float bias = 0.3f * ship->size;

  // p_left
  Vector2 mid23 = (Vector2){(p2.x + p3.x) * 0.5f, (p2.y + p3.y) * 0.5f};

  Vector2 v_left = (Vector2){mid23.x - p1.x, mid23.y - p1.y};
  float v_left_len = sqrtf(v_left.x * v_left.x + v_left.y * v_left.y);

  Vector2 dir13 = (v_left_len > 0.00001f)
                      ? (Vector2){v_left.x / v_left_len, v_left.y / v_left_len}
                      : (Vector2){1.0f, 0.0f};

  Vector2 base_left = (Vector2){mid23.x + dir13.x * forward_offset,
                                mid23.y + dir13.y * forward_offset};
  Vector2 perp_left = (Vector2){-dir13.y, dir13.x};

  Vector2 toP2 = (Vector2){p2.x - base_left.x, p2.y - base_left.y};
  float dot_left = toP2.x * perp_left.x + toP2.y * perp_left.y;
  if (dot_left > 0.0f) {
    perp_left.x = -perp_left.x;
    perp_left.y = -perp_left.y;
  }

  Vector2 p_left = (Vector2){base_left.x + perp_left.x * bias,
                             base_left.y + perp_left.y * bias};

  // p_right
  Vector2 mid13 = (Vector2){(p1.x + p3.x) * 0.5f, (p1.y + p3.y) * 0.5f};

  Vector2 v_right = (Vector2){mid13.x - p2.x, mid13.y - p2.y};
  float v_right_len = sqrtf(v_left.x * v_left.x + v_left.y * v_left.y);

  Vector2 dir23 = (v_right_len > 0.00001f) ? (Vector2){v_right.x / v_right_len,
                                                       v_right.y / v_right_len}
                                           : (Vector2){1.0f, 0.0f};

  Vector2 base_right = (Vector2){mid13.x + dir23.x * forward_offset,
                                 mid13.y + dir23.y * forward_offset};
  Vector2 perp_right = (Vector2){-dir23.y, dir23.x};

  Vector2 toP1 = (Vector2){p1.x - base_right.x, p1.y - base_right.y};
  float dot_right = toP1.x * perp_right.x + toP1.y * perp_right.y;
  if (dot_right > 0.0f) {
    perp_right.x = -perp_right.x;
    perp_right.y = -perp_right.y;
  }

  Vector2 p_right = (Vector2){base_right.x + perp_right.x * bias,
                              base_right.y + perp_right.y * bias};

  DrawTriangle(p_front, p2, p1, ship->color);
  DrawTriangle(p2, p_left, p3, ship->color);
  DrawTriangle(p1, p3, p_right, ship->color);
  DrawCircle(ship->pos.x, ship->pos.y, ship->size * 0.5, BLACK);
}

void update_ship(Ship *ship, float dt) {
  if (IsKeyDown(KEY_A))
    ship->rotation -= 3.0f * dt;
  if (IsKeyDown(KEY_D))
    ship->rotation += 3.0f * dt;

  if (IsKeyDown(KEY_W)) {
    float thrust = 350.0f;
    ship->vel.x += sinf(ship->rotation) * thrust * dt;
    ship->vel.y += -cosf(ship->rotation) * thrust * dt;
  }

  float damping = 0.99f;
  ship->vel.x *= powf(damping, dt * 60.0f);
  ship->vel.y *= powf(damping, dt * 60.0f);

  ship->pos.x += ship->vel.x * dt;
  ship->pos.y += ship->vel.y * dt;
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
  Rectangle rect = {b->pos.x, b->pos.y, b->size.x, b->size.y};

  Vector2 origin = {b->size.x * 0.5f, b->size.y * 0.5f};

  DrawRectanglePro(rect, origin, b->rotation * RAD2DEG, WHITE);
}

void calculate_ship_pos(Ship *ship) {
  ship->vel.x += ship->acc.x;
  ship->vel.y += ship->acc.y;

  ship->pos.x += ship->vel.x;
  ship->pos.y += ship->vel.y;
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "NieMe");
  SetTargetFPS(60);
  InitAudioDevice();

  Sound shoot_base = LoadSound("assets/shoot.mp3");
  SetSoundVolume(shoot_base, 0.5f);

  Sound shoot_voices[SHOOT_VOICES] = {0};
  for (int i = 0; i < SHOOT_VOICES; i++) {
    shoot_voices[i] = LoadSoundAlias(shoot_base);
    SetSoundVolume(shoot_voices[i], 0.5f);
  }

  int shoot_voice = 0;

  Ship player_ship = {
      .pos = {(float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2},
      .acc = {0, 0},
      .vel = {0, 0},
      .size = 10.0,
      .rotation = 0,
      .color = {0xDB, 0xD4, 0xCD, 0xFF}};

  int max_bullets = 100;
  Bullet *bullets = malloc(sizeof(Bullet) * max_bullets);
  int next_bullet = 0;
  for (int i = 0; i < max_bullets; i++)
    bullets[i].active = false;

  float shoot_cooldown = 0.0f;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    if (shoot_cooldown > 0.0f)
      shoot_cooldown -= dt;

    if (IsKeyDown(KEY_SPACE) && shoot_cooldown <= 0.0f) {
      Bullet *b = &bullets[next_bullet];

      float s = sinf(player_ship.rotation);
      float c = cosf(player_ship.rotation);
      Vector2 forward = (Vector2){s, -c};

      float muzzle_offset = player_ship.size * 2.0f;
      b->pos = (Vector2){player_ship.pos.x + forward.x * muzzle_offset,
                         player_ship.pos.y + forward.y * muzzle_offset};

      b->vel = (Vector2){player_ship.vel.x + forward.x * BULLET_SPEED,
                         player_ship.vel.y + forward.y * BULLET_SPEED};

      b->size = (Vector2){5.0f, 15.0f};
      b->rotation = player_ship.rotation;
      b->active = true;
      b->color = WHITE;

      next_bullet = (next_bullet + 1) % max_bullets;

      shoot_cooldown = FIRE_COOLDOWN;

      PlaySound(shoot_voices[shoot_voice]);
      shoot_voice = (shoot_voice + 1) % SHOOT_VOICES;
    }

    update_ship(&player_ship, dt);
    update_bullets(bullets, max_bullets, dt);

    BeginDrawing();
    ClearBackground((Color){0xC6, 0xC0, 0xA8, 0xFF});
    for (int i = 0; i < max_bullets; i++) {
      if (bullets[i].active)
        draw_bullet(&bullets[i]);
    }
    draw_ship(&player_ship);
    EndDrawing();
  }

  for (int i = 0; i < SHOOT_VOICES; i++)
    UnloadSoundAlias(shoot_voices[i]);
  UnloadSound(shoot_base);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
