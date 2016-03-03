entity:projectile*:base {
{
server:
    .group = group_projectile,
}

void onframe(game_t *g, entity_t *self)
{
    (void) g;

    push(self, self->vec[0]);
    if (!--self->tvar[0])
        delete(self);
}

}

entity:gadget*:base {
{
server:
    .group = group_gadget,
}

void onframe(game_t *g, entity_t *self)
{
    (void) g;

    if (!--self->tvar[0])
        delete(self);
}

}

state:buff*:base {
{
}
}

effect:fireball {
    sound {
        .loop = 0,
        .sound = snd_fireball_launch1,
    }
}

entity:fireball:projectile {
{
    .icon = tex_icon_medivh,
    .tooltip = tooltip(prophet),
    .effect = effect_fireball,
    .model = mdl_prophet,
    .modelscale = 3.0 / 16.0,
    .boundsradius = 6.0,
    .boundsheight = 18.0,
}

void onframe(game_t *g, entity_t *self)
{
    areaofeffect(g, self->pos, hero_radius + distance(5), group_hero | group_projectile,
        if (target == self || ignore(self, target))
            continue;

        if ((def(target)->group & group_hero) &&
            inrange(self->pos, target->pos, hero_radius + self->var[0] + 0.5)) {
            applypush(g, target, target->pos, self->pos, speed(160.0));
            delete(self);
            return;
        }

        if (inrange(self->pos, target->pos, self->var[0] + target->var[0] + 0.5)) {
            delete(self);
            return;
        }
    );

    super(g, self);
}

}


entity:fireball_small:projectile {
{
    .icon = tex_icon_medivh,
    .tooltip = tooltip(prophet),
    .effect = effect_fireball,
    .model = mdl_prophet,
    .modelscale = 3.0 / 16.0,
    .boundsradius = 6.0,
    .boundsheight = 18.0,
}

void onframe(game_t *g, entity_t *self)
{
    self->var[0] += speed(17.0); // < tan15*speed(128.0) / 2
    if (self->var[0] > distance(6.0))
        self->var[0] = distance(6.0);

    areaofeffect(g, self->pos, hero_radius + distance(6), group_hero | group_projectile,
        if (target == self || ignore(self, target))
            continue;

        if ((def(target)->group & group_hero) &&
            inrange(self->pos, target->pos, hero_radius + self->var[0] + 0.5)) {
            applypush(g, target, target->pos, self->pos, speed(128.0));
            delete(self);
            return;
        }

        if (inrange(self->pos, target->pos, self->var[0] + target->var[0] + 0.5)) {
            delete(self);
            return;
        }
    );

    super(g, self);
}

}

ability:fireball:spell {
{
    .icon = tex_icon_firebolt,
    .tooltip = tooltip(fireball),
    .target = tf_ground,
server:
    .cast_time = msec(400),
    .anim_time = msec(800),
    .anim = anim_spell1,
    .cooldown = cooldown_params(8000, 1000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    (void) a;
    if (t.alt) {
        spawnprojectile(g, fireball_small, self, t.pos, hero_radius, speed(128.0), 15.0);
        spawnprojectile(g, fireball_small, self, t.pos, hero_radius, speed(128.0), 0.0);
        spawnprojectile(g, fireball_small, self, t.pos, hero_radius, speed(128.0), -15.0);
    } else {
        spawnprojectile(g, fireball, self, t.pos, hero_radius, speed(128.0), 0.0);
    }
    return super(g, self, a, t);
}

}

effect:sacrifice_self {
    sound {
        .loop = 0,
        .volume = 32,
        .sound = snd_slow,
    }
}

ability:sacrifice:spell {
{
    .icon = tex_icon_cloakofflames,
    .tooltip = tooltip(sacrifice),
    .target = tf_none,
server:
    .cast_time = msec(800),
    .anim_time = msec(1200),
    .anim = anim_spell0,
    .cooldown = cooldown_params(6000, 0),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    point p;

    areaofeffect(g, self->pos, distance(16), group_hero,
        if (target == self || ignore(self, target))
            continue;

        p = target->pos;
        if (t.alt)
            teleport(target, p = mirror(self->pos, p));
        applypush(g, target, p, self->pos, speed(160.0));
    );

    playeffect(g, self, effect_sacrifice_self, ent_none, msec(2000));
    return super(g, self, a, t);
}

}

state:timeshift:buff {
{
    .icon = tex_icon_invisibility,
}

void onexpire(game_t *g, entity_t *self, state_t *s)
{
    ability_t *a;

    //playeffect(g, self, effect_thrust, ent_none, msec(3000));
    a = getability_id(self, s->aid);
    changeability(g, self, a, ability_timeshift);

    teleport(self, s->pos[0]);
}

}

ability:timeshift_activate:spell {
{
    .icon = tex_icon_cancel,
    .tooltip = tooltip(timeshift),
    .target = tf_none,
    .front_queue = 1,
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    (void) g;
    (void) t;

    expire_id(self, a->sid);
    return 0;
}

bool allowcast(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    bool res;
    uint16_t tmp;

    /* ignore the cd condition */
    tmp = a->cd;
    a->cd = 0;
    res = super(g, self, a, t);
    a->cd = tmp;

    return res;
}

}

ability:timeshift:spell {
{
    .icon = tex_icon_massteleport,
    .tooltip = tooltip(timeshift),
    .target = tf_none,
    .front_queue = 1,
server:
    .cooldown = cooldown_params(20000, 2000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    (void) t;

    state_t *s;

    s = applystate(g, self, 0, state_timeshift, 0, msec(2000), 0);
    if (!s)
        return 1;

    s->aid = a->id;
    a->sid = s->id;

    s->pos[0] = self->pos;

    changeability(g, self, a, ability_timeshift_activate);
    return super(g, self, a, t);
}

}

ability:teleport:spell {
{
    .icon = tex_icon_unsummonbuilding,
    .tooltip = tooltip(teleport),
    .target = tf_ground,
server:
    .cast_time = msec(500),
    .anim_time = msec(800),
    .anim = anim_spell0,
    .cooldown = cooldown_params(12000, 1000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    vec v;
    double m;
    point p;

    v = vec_p(self->pos, t.pos);
    m = mag(v);
    p = m < distance(192.0) ? t.pos : addvec(self->pos, scale(v, distance(192.0) / m));

    teleport(self, p);
    return super(g, self, a, t);
}

}

effect:thrust_trail {
    mdlmod {
        .len = msec(200),
        .color_start = rgba(64, 64, 64, 128),
        .color_end = rgba(64, 64, 64, 0),
    }
}

entity:thrust_trail:gadget {
{
    .model = mdl_prophet,
    .modelscale = 3.0 / 16.0,
    .effect = effect_thrust_trail,
}
}

effect:thrust {
    sound {
        .volume = 64,
        .no_follow = 1,
        .sound = snd_thunderclap,
    }
}

state:thrust:buff {
{
    .icon = tex_icon_invisibility,
}

void onframe(game_t *g, entity_t *self, state_t *s)
{
    entity_t *eff;

    (void) g;

    push(self, scale(s->vec[0], speed(384.0)));
    if (!s->tvar[0]) {
        s->tvar[0] = msec(50);

        eff = spawnunit1(g, thrust_trail, self);
        eff->tvar[0] = msec(200);
        eff->anim = self->anim;
        eff->anim.id |= 128;
    }
    s->tvar[0]--;
}

void onexpire(game_t *g, entity_t *self, state_t *s)
{
    (void) s;

    playeffect(g, self, effect_thrust, ent_none, msec(3000));
}

}

ability:thrust:spell {
{
    .icon = tex_icon_invisibility,
    .tooltip = tooltip(thrust),
    .target = tf_ground,
server:
    .cooldown = cooldown_params(14000, 2000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    state_t *s;
    vec v;

    s = applystate(g, self, ent_none, state_thrust, a->level, msec(400), 0);
    if (!s)
        return 0;

    v = vec_normp(self->pos, t.pos);

    setfacing(g, self, angle(v));
    s->vec[0] = v;

    return super(g, self, a, t);
}

}

effect:windwalk {
    sound {
        .loop = 0,
        .volume = 32,
        .sound = snd_windwalk,
    }
}

effect:windwalk_impact {
    sound {
        .loop = 0,
        .volume = 128,
        .sound = snd_roar,
    }
}

state:windwalk:buff {
{
    .icon = tex_icon_windwalkon,
server:
    .status = invisible,
}

void onexpire(game_t *g, entity_t *self, state_t *s)
{
    (void) s;

    playeffect(g, self, effect_windwalk_impact, ent_none, msec(2000));
}

void applymod(game_t *g, entity_t *self, state_t *s)
{
    self->movespeed_factor *= 1.5;
    return super(g, self, s);
}

}

ability:windwalk:spell {
{
    .icon = tex_icon_windwalkon,
    .tooltip = tooltip(windwalk),
    .target = tf_none,
    .front_queue = 1,
server:
    .cooldown = cooldown_params(18000, 1000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    if (!applystate(g, self, ent_none, state_windwalk, a->level, msec(3000 + 300 * a->level), 0))
        return 0;

    playeffect(g, self, effect_windwalk, ent_none, msec(3000));
    return super(g, self, a, t);
}

}

ability:shield:spell {
{
    .icon = tex_icon_neutralmanashield,
    .tooltip = tooltip(shield),
    .target = tf_none,
    .front_queue = 1,
server:
    .cooldown = cooldown_params(20000, 3000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    return super(g, self, a, t);
}

}

ability:doppel:spell {
{
    .icon = tex_icon_mirrorimage,
    .tooltip = tooltip(doppel),
    .target = tf_none,
    .front_queue = 1,
server:
    .cooldown = cooldown_params(18000, 1000),
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t t)
{
    return super(g, self, a, t);
}

}
