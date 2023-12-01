//constexpr int alphaTestMode;
bool alphaTest(const float alpha, const float cutoff)
{
    if (alphaTestMode == 1 && alpha >= cutoff)
        return false;
    if (alphaTestMode == 2 && alpha != cutoff)
        return false;
    if (alphaTestMode == 3 && alpha > cutoff)
        return false;
    if (alphaTestMode == 4 && alpha <= cutoff)
        return false;
    if (alphaTestMode == 5 && alpha == cutoff)
        return false;
    if (alphaTestMode == 6 && alpha < cutoff)
        return false;
    if (alphaTestMode == 7)
        return false;
    return true;
}
