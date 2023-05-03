from collections.abc import Iterable, Generator


def flatten(iterable: Iterable) -> Generator:
    """
    Генератор flatten принимает итерируемый объект iterable и с помощью обхода в глубину отдает все вложенные объекты.
    Для любых итерируемых вложенных объектов, не являющихся строками, нужно делать рекурсивный заход.
    В результате генератор должен пробегать по всем вложенным объектам на любом уровне вложенности.
    """
    if not isinstance(iterable, Iterable) or isinstance(iterable, str):
        yield iterable
        return

    for elem in iterable:
        if isinstance(elem, Iterable):
            for inner in flatten(elem):
                yield inner
        else:
            yield elem
