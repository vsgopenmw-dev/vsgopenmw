bool isDead(Particle p)
{
    return p.velocityAge.w >= p.maxAge;
}
