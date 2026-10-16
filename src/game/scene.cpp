#include "game/scene.h"

namespace swd3
{

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

// swd32.exe 0x0040F5A0: also keeps only bit 15 of the word at 0x004A9920
void Scene::FreeNodes()
{
    this->unk4ACF00.next.reset();
    this->unk4A9920 &= 0x8000;
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
    for (Particle& leaf : this->leaves)
    {
        leaf.x = 0x7FFFFFFF;
    }
}

// swd32.exe 0x0040F690: each object is destroyed (0x00430CF0) and freed
void Scene::FreeObjects()
{
    this->objects.reset();
}

}  // namespace swd3
