sources = ['cbuf.c', 'cbuf.h', 'CMakeLists.txt', 'hes.c', 'hes.h', 'interface.c', 'interface.h', 'main.cpp', 'radio.c', 'radio.h', 'uart.c', 'uart.h', 'uart_rx.pio', 'uart_tx.pio']
arcana_sources = ['default5x7.h', 'lcd_draw.c', 'lcd_driver.pio', 'lcd_hl.h', 'lcd_misc.c', 'lcd_text.c', 'lcd_util.h']

sources += ['arcana_lcd_rp2040/' + s for s in arcana_sources]

line_count = 0
sloc_count = 0
for s in sources:
    lines = open(s).readlines()
    # print(f'{s}: {lines} lines')
    line_count += len(lines)
    for l in lines:
         if len(l.strip())!=0 and l.strip()[0:1]!='//':
             sloc_count += 1

print(f'Total lines: {line_count}')
print(f'Total sloc: {sloc_count}')