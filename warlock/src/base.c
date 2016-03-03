entity:base* {
{
    .model = -1,
    .icon = -1,
    .effect = 0xFFFF,
    .tooltip = tooltip_none,
}

void onframe(game_t *g, entity_t *self)
{
    (void) g;
    (void) self;
}

void onspawn(game_t *g, entity_t *self)
{
    (void) g;
    (void) self;
}

void ondeath(game_t *g, entity_t *self, entity_t *source)
{
    (void) g;
    (void) self;
    (void) source;
}

bool canmove(game_t *g, entity_t *self)
{
    (void) g;

    return !(self->status & immobilized);
}

bool visible(game_t *g, entity_t *self, entity_t *ent)
{
    (void) g;

    return (!(self->status & invisible) || (ent->status & truesight));
}

uint32_t ondamaged(game_t *g, entity_t *self, entity_t *source, uint32_t amount, uint8_t type)
{
    (void) g;
    (void) source;

    double x;

    type >>= 1;

    if (type == 2)
        return amount;

    x = self->armor[type] * 0.06;
    return (1.0 - x / (1.0 + fabs(x))) * amount + 0.5;
}

void initmod(game_t *g, entity_t *ent)
{
    (void) g;

    ent->status = 0;
    ent->movespeed = def(ent)->movespeed;
    ent->movespeed_factor = 1.0;
}

void finishmod(game_t *g, entity_t *ent)
{
    (void) g;

    ent->vision = def(ent)->vision;
    ent->maxhp = def(ent)->hp;
    ent->maxmana = def(ent)->mana;
    ent->movespeed *= ent->movespeed_factor;
}
}

entity:hero*:base {
{
    .uiflags = ui_control | ui_target,
server:
    .group = group_hero,
    .control = 1,
}
}

ability:base* {
{
    .icon = 0xFFFF,
    .target = 0,
    .tooltip = tooltip_none,
server:
    .range = distance_max,
    .range_max = distance_max,
}

void onframe(game_t *g, entity_t *self, ability_t *a)
{
    (void) g;
    (void) self;
    (void) a;
}

bool allowcast(game_t *g, entity_t *self, ability_t *a, target_t target)
{
    (void) g;
    (void) self;
    (void) target;

    return (a->cd == 0);
}

cast_t startcast(game_t *g, entity_t *self, ability_t *a, target_t *target)
{
    (void) g;

    if (target->type != target_none) {
        if (!inrange(self->pos, target->pos, def(a)->range))
            return cast_norange(def(a)->range);

        if (!facing(self->angle, getangle(self->pos, target->pos), 8192 *2)) /* +/- 90degrees */
            return cast_noangle;
    }

    return cast(def(a)->anim, def(a)->cast_time, def(a)->anim_time);
}

bool continuecast(game_t *g, entity_t *self, ability_t *a, target_t *target)
{
    (void) g;

    if (target->type != target_none) {
        if (!inrange(self->pos, target->pos, def(a)->range_max))
            return 0;
    }

    return 1;
}

void onpreimpact(game_t *g, entity_t *self, ability_t *a, target_t target)
{
    (void) g;
    (void) self;
    (void) a;
    (void) target;
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t target)
{
    (void) g;
    (void) self;
    (void) a;
    (void) target;
    return 0;
}

bool ondeath(game_t *g, entity_t *self, ability_t *a, entity_t *source)
{
    (void) g;
    (void) self;
    (void) a;
    (void) source;
    return 0;
}

void onstateexpire(game_t *g, entity_t *self, ability_t *a, state_t *s)
{
    (void) g;
    (void) self;
    (void) a;
    (void) s;
}

void onabilitystart(game_t *g, entity_t *self, ability_t *a, ability_t *b, target_t target)
{
    (void) g;
    (void) self;
    (void) a;
    (void) b;
    (void) target;
}

void onabilitypreimpact(game_t *g, entity_t *self, ability_t *a, ability_t *b, target_t target)
{
    (void) g;
    (void) self;
    (void) a;
    (void) b;
    (void) target;
}

void finishmod(game_t *g, entity_t *self, ability_t *a)
{
    (void) g;
    (void) self;
    (void) a;
}

}

ability:spell*:base {
{
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    (void) t;

    startcooldown(g, self, a);
    return 0;
}

void finishmod(game_t *g, entity_t *self, ability_t *a)
{
    setcooldown(g, self, a, def(a)->cooldown.base - a->level * def(a)->cooldown.perlevel);
}

}

state:base* {
{
    .effect = -1,
    .icon = 0xFFFF,
    .tooltip = tooltip_none,
}

void onframe(game_t *g, entity_t *self, state_t *s)
{
    (void) g;
    (void) self;
    (void) s;
}

void onexpire(game_t *g, entity_t *self, state_t *s)
{
    (void) g;
    (void) self;
    (void) s;
}

void onabilityimpact(game_t *g, entity_t *self, state_t *s, ability_t *a)
{
    (void) g;
    (void) self;
    (void) s;
    (void) a;
}

void onanimfinish(game_t *g, entity_t *self, state_t *s)
{
    (void) g;
    (void) self;
    (void) s;
}

bool ondeath(game_t *g, entity_t *self, state_t *s, entity_t *source)
{
    (void) g;
    (void) self;
    (void) s;
    (void) source;

    return 0;
}

void applymod(game_t *g, entity_t *self, state_t *s)
{
    (void) g;

    if (s->def < 0)
        return;

    self->status |= def(s)->status;
}

void ondamage(game_t *g, entity_t *self, state_t *s, entity_t *target, uint32_t amount, uint8_t type)
{
    (void) g;
    (void) self;
    (void) s;
    (void) target;
    (void) amount;
    (void) type;
}

uint32_t ondamaged(game_t *g, entity_t *self, state_t *s, entity_t *source, uint32_t amount, uint8_t type)
{
    (void) g;
    (void) self;
    (void) s;
    (void) source;
    (void) type;

    return amount;
}

}
