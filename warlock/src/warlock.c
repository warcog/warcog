ability:unready:base {
{
    .icon = tex_icon_cancel,
    .tooltip = tooltip(unready),
    .target = tf_none,
    .front_queue = 1,
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t target)
{
    (void) target;

    changeability(g, self, a, ability_ready);
    return 0;
}

}

ability:ready:base {
{
    .icon = tex_icon_hire,
    .tooltip = tooltip(ready),
    .target = tf_none,
    .front_queue = 1,
}

bool onimpact(game_t *g, entity_t *self, ability_t *a, target_t target)
{
    (void) target;

    changeability(g, self, a, ability_unready);
    movecam(g, self->owner, position(64, 64));
    return 0;
}

}

state:dead:base {
{
    .icon = tex_icon_deathcoil,
    .tooltip = tooltip(dead),
server:
    .status = dead,
}
}

state:shop:base {
{
    .icon = tex_icon_goldmine,
    .tooltip = tooltip(shop),
server:
    .status = dead,
}
}

effect:firetrail {
    particle {
        .bone = mdl_prophet_Staff,
        .rate = msec(50),
        .start_frame = 0,
        .end_frame = 63,
        .lifetime = msec(300),
        .start_size = 16.0,
        .end_size = 32.0,
        .texture = tex_clouds_8x8_fire,
        .psize = 0,
        .color = rgba(255, 80, 0, 255),

        .x = 70.0,
    }
}

entity:warlock:hero {
{
    .icon = tex_icon_medivh,
    .tooltip = tooltip(prophet),
    .effect = effect_firetrail,
    .model = mdl_prophet,
    .modelscale = 3.0 / 16.0,
    .boundsradius = 6.0,
    .boundsheight = 18.0,
    .voice = {
        {4, {snd_necro1_what1, snd_necro1_what2, snd_necro1_what3, snd_necro1_what4}},
        {4, {snd_necro1_yes1, snd_necro1_yes2, snd_necro1_yes3, snd_necro1_yes4}},
        {3, {snd_necro1_attack1, snd_necro1_attack2, snd_necro1_attack3}},
        {5, {snd_necro1_pissed1, snd_necro1_pissed2, snd_necro1_pissed3,
            snd_necro1_pissed4, snd_necro1_pissed5}},
    },

server:
    .vision = distance(128),
    .hp = hp(100),
    .mana = mana(100),

    .movespeed = speed(36.0),
    .turnrate = turnrate(65536),

    .anim = {
        {1, {mdl_prophet_Stand}}, /* idle */
        {2, {mdl_prophet_Stand_Second, mdl_prophet_Stand_third}}, /* bored */
        {1, {mdl_prophet_Walk}}, /* walk */
        {1, {mdl_prophet_Death}}, /* death */
        {0, {0}}, /* spawn */
        {0, {0}}, /* attack */
        {0, {0}}, /* idle-disabled */
        {0, {0}}, /* taunt */

        {1, {mdl_prophet_Stand_First}}, /* (spell0) */
        {1, {mdl_prophet_Spell_Attack}}, /* (spell1) */
        {1, {mdl_prophet_Dissipate}}, /* (spell2) */
    },

    .idle_time = msec(1000),
    .bored_time = msec(2000),
    .walk_time = msec(1000),
}

void onspawn(game_t *g, entity_t *self)
{
    giveability(g, self, ability_ready);

    giveability(g, self, ability_fireball);
    giveability(g, self, ability_sacrifice);
    giveability(g, self, ability_timeshift);
    giveability(g, self, ability_teleport);
    giveability(g, self, ability_thrust);
    giveability(g, self, ability_windwalk);
    giveability(g, self, ability_shield);
    giveability(g, self, ability_doppel);

    applystate(g, self, ent_none, state_dead, 0, permanent, 0);
    super(g, self);
}

void onframe(game_t *g, entity_t *self)
{
    (void) g;

    push(self, scale(self->vec[1], self->var[1]));

    //refresh(g, self);

    /* out of bounds */
    //d = delta(tiles(32, 32), self->pos);
    //if (abs(d.x) >= tile(game.size + 1) || abs(d.y) >= tile(game.size + 1))
    //    damage(self, 0, hpsec(10));
}

void finishmod(game_t *g, entity_t *self)
{
    double dec = accel(128.0);

    self->vec[1] = self->vec[0];
    self->var[1] = self->var[0];

    self->var[0] -= dec;
    if (self->var[0] < 0.0)
        self->var[0] = 0.0;

    return super(g, self);
}

void ondeath(game_t *g, entity_t *self, entity_t *src)
{
    (void) g;
    (void) self;
    (void) src;

    /*state_t *s;

    s = getstate(self, s_timeshift);
    if (s) {
        expire(s);
    } else {
        if (!getstate(self, s_dead))
            applystate(self, s_dead, 0);
        setmodifier(self, dead);
        refresh(self);
    }*/
}

}
