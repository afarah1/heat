static double
max(double **surface, uint32_t n)
{
  double ans = surface[0][0];
  for (uint32_t i = 0; i < n; i++)
    for (uint32_t j = 0; j < n; j++)
      if (surface[i][j] > ans)
        ans = surface[i][j];
  return ans;
}
