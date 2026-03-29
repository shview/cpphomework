#include "Snake.h"
#include <algorithm>

Snake::Snake() { reset(); }

void Snake::initSprites(const sf::Texture& hTex, const sf::Texture& bTex) {
    headSprite.emplace(hTex);
    bodySprite.emplace(bTex);
    headSprite->setOrigin({ hTex.getSize().x / 2.0f, hTex.getSize().y / 2.0f });
    bodySprite->setOrigin({ bTex.getSize().x / 2.0f, bTex.getSize().y / 2.0f });
}

void Snake::reset() {
    headPos = { 400.f, 450.f };
    currentDir = { 1.f, 0.f };
    bodyCount = 2;
    health = 3;
    energy = 100.f;
    dashCharges = 2;
    dashCooldown = 0.f;
    trail.clear();
    isDashing = false;
    isInvincible = false;
    parseProgress = 0.f;
    isParsing = false;
    isSlowing = false;
}

void Snake::takeDamage() {
    health--;
    isInvincible = true;
    invTimer = 1.0f;
    bodyCount = std::max(2, bodyCount - 2);
}

bool Snake::update(float dt, sf::Vector2u winSize) {
    if (isInvincible) {
        invTimer -= dt;
        if (invTimer <= 0.f) isInvincible = false;
    }

    energy = std::min(100.f, energy + 12.f * dt);

    if (dashCharges < 2) {
        dashCooldown += dt;
        if (dashCooldown >= 2.5f) { dashCharges++; dashCooldown = 0.f; }
    }

    float currentMoveSpeed = normalSpeed;
    bool atEdge = (headPos.x <= 15.f || headPos.x >= (float)winSize.x - 15.f ||
        headPos.y <= 15.f || headPos.y >= (float)winSize.y - 15.f);

    if (isDashing) {
        currentMoveSpeed = dashSpeed;
        dashTimer -= dt;
        if (dashTimer <= 0.f) isDashing = false;
    }
    else {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || atEdge) {
            currentMoveSpeed = slowSpeed;
            isSlowing = true;
            isParsing = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        }
        else {
            isSlowing = false;
            isParsing = false;
        }
    }

    sf::Vector2f prevPos = headPos;

    if (!isDashing) {
        sf::Vector2f inputDir(0.f, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))         inputDir.y = -1.f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  inputDir.y = 1.f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  inputDir.x = -1.f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) inputDir.x = 1.f;

        bool hasInput = (inputDir.x != 0.f || inputDir.y != 0.f);
        if (hasInput) currentDir = inputDir;

        if (!isSlowing || hasInput) headPos += currentDir * currentMoveSpeed * dt;
    }
    else {
        headPos += currentDir * currentMoveSpeed * dt;
    }

    if (headSprite) {
        float angle = 0.f;
        if (currentDir.x > 0.f) angle = 0.f;
        else if (currentDir.x < 0.f) angle = 180.f;
        else if (currentDir.y > 0.f) angle = 90.f;
        else if (currentDir.y < 0.f) angle = -90.f;
        headSprite->setRotation(sf::degrees(angle));
    }

    headPos.x = std::clamp(headPos.x, 15.f, (float)winSize.x - 15.f);
    headPos.y = std::clamp(headPos.y, 15.f, (float)winSize.y - 15.f);

    float distMoved = std::hypot(headPos.x - prevPos.x, headPos.y - prevPos.y);
    if (distMoved > 0.1f) {
        int steps = static_cast<int>(std::ceil(distMoved / 3.0f));
        for (int i = 1; i <= steps; ++i) {
            trail.push_front({ prevPos + (headPos - prevPos) * ((float)i / steps) });
        }
    }

    if (trail.size() > (size_t)(maxBody + 2) * gap) trail.resize((maxBody + 2) * gap);

    if (isParsing && bodyCount >= 3) {
        parseProgress += (30.f + (bodyCount - 3) * 15.f) * dt;
        if (parseProgress >= 100.f) {
            parseProgress = 0.f;
            bodyCount = 2;
            trail.clear();
            return true; // 触发发射
        }
    }
    else {
        parseProgress = std::max(0.f, parseProgress - 50.f * dt);
    }
    return false;
}

void Snake::draw(sf::RenderWindow& window) {
    if (!headSprite || !bodySprite) return;

    if (isDashing) {
        for (size_t i = 0; i < std::min((size_t)15, trail.size()); i += 3) {
            sf::Sprite ghost = *headSprite;
            ghost.setPosition(trail[i].position);
            ghost.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(120 - i * 6)));
            window.draw(ghost);
        }
    }

    if (!isSlowing) {
        for (int i = 1; i <= bodyCount; ++i) {
            size_t idx = i * gap;
            if (idx < trail.size()) {
                bodySprite->setPosition(trail[idx].position);
                // 根据轨迹计算身体方向
                if (idx + 1 < trail.size()) {
                    sf::Vector2f dir = trail[idx].position - trail[idx + 1].position;
                    float angle = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
                    bodySprite->setRotation(sf::degrees(angle));
                }
                window.draw(*bodySprite);
            }
        }
    }

    headSprite->setPosition(headPos);
    if (isInvincible && static_cast<int>(invTimer * 15) % 2 == 0) headSprite->setColor(sf::Color(255, 255, 255, 0));
    else headSprite->setColor(isParsing ? sf::Color(255, 255, 0) : sf::Color::White);
    window.draw(*headSprite);

    if (isParsing && bodyCount >= 3) {
        sf::RectangleShape bar({ 40.f * (parseProgress / 100.f), 4.f });
        bar.setPosition({ headPos.x - 20.f, headPos.y - 25.f });
        bar.setFillColor(sf::Color::Yellow);
        window.draw(bar);
    }
}