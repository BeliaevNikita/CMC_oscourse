/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/trap.h>
#include <kern/picirq.h>

/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 * NOTE: CMOS_CMD is the same port that is used to toggle NMIs,
 * so nmi_disable() cannot be used. And you have to use provided
 * constant.
 *
 * Why it is necessary?
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */
    // LAB 4: Your code here

    outb(CMOS_CMD, reg | CMOS_NMI_LOCK);
    uint8_t res = inb(CMOS_DATA);

    nmi_enable();
    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here

    outb(CMOS_CMD, reg | CMOS_NMI_LOCK);
    outb(CMOS_DATA, value);

    nmi_enable();
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

void
rtc_timer_pic_interrupt(void) {
    // LAB 4: Your code here
    // Enable PIC interrupts.

    // После инициализации часов RTC и программируемого контроллера прерываний PIC
    // в функции rtc_timer_pic_interrupt() необходимо размаскировать на контроллере
    // линию IRQ_CLOCK, по которой приходят прерывания от часов. Для этого можно
    // использовать функцию pic_irq_unmask(). 
    pic_irq_unmask(IRQ_CLOCK);
}

void
rtc_timer_pic_handle(void) {
    // После того, как прерывание сгенерировано и обработано, перед вызовом планировщика необходимо прочесть регистр статуса RTC...
    // ... и отправить сигнал EOI на контроллер прерываний, сигнализируя об окончании обработки прерывания.
    // Иначе дальнейшие прерывания от часов не будут генерироваться PIC.
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);
}

void
rtc_timer_init(void) {
    // LAB 4: Your code here
    // (use cmos_read8()/cmos_write8())

    // 1. Переключение на регистр часов B.
    // 2. Чтение значения регистра B из порта ввода-вывода.
    uint8_t register_b = cmos_read8(RTC_BREG);
    // 3. Установка бита RTC_PIE.
    register_b |= RTC_PIE;
    // 4. Запись обновленного значения регистра в порт ввода-вывода.
    cmos_write8(RTC_BREG, register_b);

    // В данный момент часы реального времени работают на некой стандартной частоте.
    // Измените процедуру rtc_timer_init так, чтобы прерывания от часов приходили один раз в полсекунды.
    // Для этого необходимо изменить делитель частоты, которому соответствуют младшие 4 бита регистра часов A.

    // Page 14/29
    // RS3 | RS2 | RS1 | RS0 | Frequency | Units | Period | Units
    // 1   | 1   | 1   | 1   | 2         | Hz    | 500    | ms
    uint8_t register_a = cmos_read8(RTC_AREG);
    const uint8_t RS_2_HZ_VALUE = 0b1111u;
    register_a |= RS_2_HZ_VALUE;

    // 1101 for test. 125 ms
    // uint8_t register_a = cmos_read8(RTC_AREG);
    // register_a |= 0b00001101u;
    // register_a &= 0b11111101u;

    // 4. Запись обновленного значения регистра в порт ввода-вывода.
    cmos_write8(RTC_AREG, register_a);
}

uint8_t
rtc_check_status(void) {
    // LAB 4: Your code here
    // (use cmos_read8())

    // Для проверки статуса часов в функции rtc_check_status() необходимо прочитать значение регистра часов C.
    uint8_t register_c = cmos_read8(RTC_CREG);
    return register_c;
}
