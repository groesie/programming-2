from collections import OrderedDict, namedtuple


class CacheInfo(namedtuple('CacheInfo', ['cache_hits', 'cache_misses'])):
    def __repr__(self):
        return f'CacheInfo(hits={self.cache_hits}, misses={self.cache_misses})'


class lru_cache:
    def __init__(self, max_items: int):
        self.cache = OrderedDict()
        self.stats = CacheInfo(0, 0)
        self.max_items = max_items
        self.function = None

    def __call__(self, *args, **kwargs):
        if self.function is None:
            self.function = args[0]
            self.__name__ = self.function.__name__
            self.__doc__ = self.function.__doc__
            self.__module__ = self.function.__module__
            return self
        
        key = str(args)
        if kwargs:
            key += str(sorted(kwargs.items()))
        if key in self.cache:
            value = self.cache.pop(key)
            self.cache[key] = value
            self.stats = CacheInfo(self.stats.cache_hits + 1, self.stats.cache_misses)
        else:
            self.stats = CacheInfo(self.stats.cache_hits, self.stats.cache_misses + 1)
            value = self.function(*args, **kwargs)
            self.cache[key] = value
            if len(self.cache) > self.max_items:
                self.cache.popitem(last=False)
        return value
