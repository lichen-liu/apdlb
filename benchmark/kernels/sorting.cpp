void sorting_kernel(size_t i)
{
    constexpr size_t offset = 1;
    constexpr size_t scale = 25000;
    const size_t n = offset + i * scale;

    std::mt19937 mersenne_engine;
    std::uniform_real_distribution<float> dist{0, 1.0};
    auto gen = [&dist, &mersenne_engine]()
    {
        return dist(mersenne_engine);
    };
    std::vector<float> vec(n);
    std::generate(vec.begin(), vec.end(), gen);

    std::sort(vec.begin(), vec.end());
    // printf("task=%lu size=%lu done\n", i, vec.size());
}