#pragma once

#include <memory>
#include <future>
#include <unordered_map>

namespace bstorm
{
// SharedCache: flyweight pattern, 非同期ロード機能付き
// NOTE: メインスレッド終了前に破棄しなければならない

// K: require operator==, should be copyable

template <class K, class V>
class CacheStore
{
private:
    template <class V>
    struct CacheEntry
    {
        CacheEntry(bool reserve, std::shared_future<std::shared_ptr<V>>& future) :
            isReserved(reserve),
            future(future)
        {
        }
        bool isReserved;
        std::shared_future<std::shared_ptr<V>> future;
    };
    std::unordered_map<K, CacheEntry<V>> cacheMap_;
public:
    // blocking
    const std::shared_ptr<V>& Get(const K& key) const noexcept(false)
    {
        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end())
        {
            return it->second.future.get();
        }
        throw std::runtime_error("cache not exist.");
    }

    bool Contains(const K& key) const
    {
        return cacheMap_.count(key) != 0;
    }

    bool IsLoadCompleted(const K& key) const
    {
        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end())
        {
            return it->second.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }
        return false;
    }

    template <class... Args>
    const std::shared_ptr<V>& Load(const K& key, Args&&... args) noexcept(false)
    {
        return LoadAsync(key, std::forward<Args>(args)...).get();
    }

    template <class... Args>
    const std::shared_future<std::shared_ptr<V>>& LoadAsync(const K& key, Args&&... args) noexcept(true)
    {
        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end())
        {
            return it->second.future;
        }

        cacheMap_.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(false, std::async(std::launch::async | std::launch::deferred, [args...]()
        {
            return std::make_shared<V>(args...);
        }).share()));
        return cacheMap_.at(key).future;
    }

    void Remove(const K& key)
    {
        cacheMap_.erase(key);
    }

    void RemoveAll()
    {
        // 中身を空にしてメモリを解放
        std::unordered_map<K, CacheEntry<V>>().swap(cacheMap_);
    }

    // non blocking
    void RemoveUnused()
    {
        auto it = cacheMap_.begin();
        while (it != cacheMap_.end())
        {
            const auto& entry = it->second;
            try
            {
                if (!entry.isReserved)
                {
                    if (entry.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    {
                        const auto& cache = entry.future.get();
                        if (cache.use_count() <= 1)
                        {
                            cacheMap_.erase(it++);
                            continue;
                        }
                    }
                }
            } catch (...) {}
            ++it;
        }
    }

    void SetReserveFlag(const K& key, bool reserve)
    {
        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end())
        {
            auto& entry = it->second;
            entry.isReserved = reserve;
        }
    }

    bool IsReserved(const K& key) const
    {
        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end())
        {
            auto& entry = it->second;
            return entry.isReserved;
        }
        return false;
    }

    // non blocking
    template <class Fn>
    void ForEach(Fn func)
    {
        for (auto& pair : cacheMap_)
        {
            const K& key = pair.first;
            CacheEntry<V>& entry = pair.second;
            bool& reserved = entry.isReserved;
            auto& future = entry.future;
            if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                const std::shared_ptr<V>& cache = future.get();
                func(pair.first, reserved, cache);
            }
        }
    }
};
};