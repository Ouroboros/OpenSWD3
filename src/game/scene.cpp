#include "game/scene.h"

namespace swd3
{

namespace
{

constexpr i16 kHeatLength = 4;  // Scene::heatLengths after ResetEffects

}  // namespace

// swd32.exe 0x0040F500
void Scene::FreeEffects()
{
    this->effects.reset();
}

// swd32.exe 0x0040F540
void Scene::FreeSpritesA()
{
    this->spritesA.reset();
}

// swd32.exe 0x0040F570
void Scene::FreeSpritesB()
{
    this->spritesB.reset();
}

// swd32.exe 0x0040F5A0: the talk windows freed; only bit 15 of the word at 0x004A9920 kept
void Scene::FreeNodes()
{
    this->talkWindows.reset();
    this->playerHold &= Scene::HoldLocked;
}

// swd32.exe 0x0040F5E0
void Scene::FreeAnimations()
{
    this->animationsA.reset();
    this->animationsB.reset();
}

// swd32.exe 0x0040F630
void Scene::FreeMarks()
{
    for (RoleMark& mark : this->marks)
    {
        mark = RoleMark{};
    }
}

// swd32.exe 0x0040F670
void Scene::ResetLeaves()
{
    for (MapParticle& leaf : this->leaves)
    {
        leaf.x = MapParticle::Unplaced;
    }
}

// swd32.exe 0x0040F690: each object is destroyed (0x00430CF0) and freed
void Scene::FreeObjects()
{
    this->objects.reset();
}

// swd32.exe 0x00411D00: the effects of the scene ended before a game is loaded: the lists freed, the fade stopped (its
// steps 0), the quake cleared and the heat shimmer reset (the original clears 0x100 bytes from 0x004AC9BC)
void Scene::ResetEffects()
{
    this->FreeEffects();
    this->FreeSpritesA();
    this->FreeSpritesB();
    this->FreeNodes();
    this->FreeAnimations();
    this->FreeMarks();
    this->ResetLeaves();
    this->fade.stepBlue = 0;
    this->shake.fill(Scene::ShakeEnd);
    this->fade.stepGreen = 0;
    this->heatCounts.fill(1);
    this->fade.stepRed = 0;
    this->heatLengths.fill(kHeatLength);
    this->heatFrame = 0;
    this->heatOffsets.fill(0);
}

}  // namespace swd3
