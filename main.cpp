#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

	int GetHP() const {
		return hp;
	}

	bool TakeDamage(int dmg) {
		hp -= dmg;
		if (hp <= 0) {
			return true;
		}
		int newSize = GetSizeForHP();
		if (newSize < static_cast<int>(render.size) && newSize >= 1) {
			render.size = static_cast<Renderable::Size>(newSize);
		}
		return false;
	}

	int GetOriginalSize() const {
		return originalSize;
	}

protected:
	void init(int screenW, int screenH) {
		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));
		originalSize = static_cast<int>(render.size); // zapisz oryginalny rozmiar
		hp = GetMaxHPForSize(static_cast<int>(render.size));

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
			screenW * 0.5f + cosf(ang) * rad,
			screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	int GetMaxHPForSize(int size) const {
		switch (size) {
		case 4: return 200; // LARGE
		case 2: return 150; // MEDIUM
		case 1: return 75; // SMALL
		default: return 25;
		}
	}
	int GetSizeForHP() const {
		if (hp > 150) return 4; // LARGE
		if (hp > 75) return 2; // MEDIUM
		return 1;              // SMALL
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	int hp = 1;
	int originalSize = 1; // NOWE POLE
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	default: {
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)));
	}
	}
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, TRIPLE, COUNT };

constexpr int LASER_DAMAGE = 20;
constexpr int BULLET_DAMAGE = 40;
constexpr int TRIPLE_DAMAGE = 15;

class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
		switch (type) {
		case WeaponType::BULLET:
			DrawCircleV(transform.position, 5.f, WHITE);
			break;
		case WeaponType::TRIPLE:
			DrawCircleV(transform.position, 4.f, WHITE);
			break;
		case WeaponType::LASER:
		{
			static constexpr float LASER_LENGTH = 30.f;
			Rectangle lr = { transform.position.x - 2.f, transform.position.y - LASER_LENGTH, 4.f, LASER_LENGTH };
			DrawRectangleRec(lr, RED);
			break;
		}
		default:
			break;
		}
	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		float value;

		switch (type) {
		case WeaponType::BULLET:
			value = 5.f;
			break;
		case WeaponType::LASER:
			value = 2.f;
			break;
		case WeaponType::TRIPLE:
			value = 2.f;
			break;
		default:
			value = 0.f;
			break;
		}

		return value;

	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
};

// Poprawiona fabryka pocisków:
inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	float speed)
{
	Vector2 vel{ 0, -speed };
	switch (wt) {
	case WeaponType::LASER:
		return Projectile(pos, vel, LASER_DAMAGE, wt);
	case WeaponType::BULLET:
		return Projectile(pos, vel, BULLET_DAMAGE, wt);
	case WeaponType::TRIPLE:
		return Projectile(pos, vel, TRIPLE_DAMAGE, wt);
	default:
		return Projectile(pos, vel, 1, wt);
	}
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 12.f; // shots/sec
		fireRateBullet = 11.f;
		fireRateTriple = 5.f; 
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
		spacingTriple = 30.f; 
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		if (wt == WeaponType::LASER) return fireRateLaser;
		if (wt == WeaponType::BULLET) return fireRateBullet;
		return fireRateTriple;
	}

	float GetSpacing(WeaponType wt) const {
		if (wt == WeaponType::LASER) return spacingLaser;
		if (wt == WeaponType::BULLET) return spacingBullet;
		return spacingTriple;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      fireRateTriple;
	float      spacingLaser;
	float      spacingBullet;
	float      spacingTriple;
};

class PlayerShip : public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		currentTextureIndex = 0;
		LoadCurrentTexture();
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}

		if (IsKeyPressed(KEY_L)) {
			currentTextureIndex = 1 - currentTextureIndex;
			UnloadTexture(texture);
			LoadCurrentTexture();
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
			transform.position.x - (texture.width * scale) * 0.5f,
			transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}

private:
	void LoadCurrentTexture() {
		const char* textureFiles[2] = { "spaceship1.png", "spaceship2.jpg" };
		texture = LoadTexture(textureFiles[currentTextureIndex]);
		GenTextureMipmaps(&texture);
		SetTextureFilter(texture, 2);

		constexpr float TARGET_SHIP_WIDTH = 80.0f; // docelowa szerokość statku w px
		scale = TARGET_SHIP_WIDTH / static_cast<float>(texture.width);
	}

	Texture2D texture;
	float     scale;
	int       currentTextureIndex;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;
		score = 0; // reset punktów na start

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

			// Update player
			player->Update(dt);

			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
				score = 0; // reset punktów po restarcie
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::RANDOM;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();

						if (currentWeapon == WeaponType::TRIPLE) {
							constexpr float angleOffsets[3] = { -15.0f * (PI / 180.0f), 0.0f, 15.0f * (PI / 180.0f) };
							for (float angleOffset : angleOffsets) {
								Vector2 dir = { 0.0f, -1.0f };
								float cosA = cosf(angleOffset);
								float sinA = sinf(angleOffset);
								Vector2 rotatedDir = {
									dir.x * cosA - dir.y * sinA,
									dir.x * sinA + dir.y * cosA
								};
								Vector2 vel = Vector2Scale(rotatedDir, projSpeed);
								projectiles.push_back(Projectile(p, vel, TRIPLE_DAMAGE, WeaponType::TRIPLE));
							}
						} else {
							projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed));
						}
						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}

			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {
						if ((*ait)->TakeDamage(pit->GetDamage())) {
							int origSize = (*ait)->GetOriginalSize();
							if (origSize == 4) score += 10;
							else if (origSize == 2) score += 5;
							else if (origSize == 1) score += 2;
							ait = asteroids.erase(ait);
						}
						else {
							++ait;
						}
						pit = projectiles.erase(pit);
						removed = true;
						break;
					}
				}
				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
				Renderer::Instance().Begin();

				DrawText(TextFormat("HP: %d", player->GetHP()),
					10, 10, 20, GREEN);

				DrawText(TextFormat("Score: %d", score),
					10, 70, 20, YELLOW);

				const char* weaponName;

				switch (currentWeapon) {
				case WeaponType::LASER:
					weaponName = "LASER";
					break;
				case WeaponType::BULLET:
					weaponName = "BULLET";
					break;
				case WeaponType::TRIPLE:
					weaponName = "TRIPLE";
					break;
				default:
					weaponName = "UNKNOWN";
					break;
				}

				DrawText(TextFormat("Weapon: %s", weaponName),
					10, 40, 20, BLUE);

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();

				Renderer::Instance().End();
			}
		}
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
		score = 0;
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;
	int score; // <-- dodane pole

	static constexpr int C_WIDTH = 1280;
	static constexpr int C_HEIGHT = 720;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
};

int main() {
	Application::Instance().Run();
	return 0;
}
