# button_input

**Абстракция** пользовательской кнопки SOS: одно логическое нажатие на физическое (с антидребезгом в драйвере).

## API

```c
typedef struct button_input {
    const char *name;
    button_input_init_fn init;
    button_input_deinit_fn deinit;
    button_input_poll_pressed_fn poll_pressed;  // true = одно событие с прошлого опроса
    void *impl;
} button_input_t;
```

## Протокол (логический)

| Операция | Поведение |
|----------|-----------|
| `init` | Настройка GPIO (pull-up, режим input) |
| `poll_pressed` | **Edge-detected** press: `true` один раз после устойчивого нажатия |
| `deinit` | Опционально (не используется в `gpio_button`) |

Физический уровень (active-low, debounce 30 ms) — в [gpio_button](../drivers/gpio_button/README.md).

## Потребители

- [button_notify](../button_notify/README.md) — фоновый опрос 10 ms.
- Экземпляр выдаёт [button_registry](../button_registry/README.md).

## Файлы

| Файл | Роль |
|------|------|
| `button_input.h` | Vtable и inline helpers |
