# feedback_output

**Абстракция** пользовательского индикатора (свет, вибромотор): единый API «задать ритм» + «тикнуть таймер».

## Принцип работы

```c
typedef struct feedback_output {
    const char *name;
    feedback_output_init_fn init;
    feedback_output_deinit_fn deinit;
    feedback_output_set_period_fn set_blink_period_ms;
    feedback_output_tick_fn tick;
    void *impl;
} feedback_output_t;
```

| Метод | Смысл |
|-------|--------|
| `set_blink_period_ms(period_ms)` | Полный цикл «вкл + выкл»; **0** — индикатор выключен |
| `tick()` | Шаг генератора (вызывается часто из `distance_feedback`) |

Прикладной слой не знает GPIO: реализация подставляется через [feedback_registry](../feedback_registry/README.md) (сейчас LED).

## Семантика period_ms

- Один период = фаза ON + фаза OFF (duty 50% в `feedback_led`).
- Чем **меньше** period — тем **чаще** вспышки («срочнее» препятствие).
- Значение задаёт `distance_feedback` по зоне дистанции и hazard.

## Расширение на вибромотор

Новый драйвер: те же callback'и; `period_ms` интерпретируется как «бзз–пауза». Замена только в `feedback_registry.c`.

## Файлы

| Файл | Роль |
|------|------|
| `feedback_output.h` | Vtable и inline API |
