from pathlib import Path
import hashlib
import json
import re

WORKSPACE = Path(r'D:\stm32halproject\stm32f407vgt6\uart')
TARGET = Path(r'D:\gk_rm_demo\iic_demo')
STAGE = WORKSPACE / 'outputs/oled-demo-prepared/iic_demo'

# 只沿用原工程完整的 95 个 8x16 ASCII 字形，不引入中文/滚动接口。
original_font = (WORKSPACE / 'Modules/OLED/oledfont.h').read_text(encoding='utf-8', errors='replace')
match = re.search(r'const unsigned char asc2_1608\[\]\[16\]\s*=\s*\{(.*?)\};', original_font, re.S)
assert match is not None
glyphs = re.findall(r'\{([^{}]+)\}', match.group(1))
assert len(glyphs) == 95
assert all(len(re.findall(r'0x[0-9a-fA-F]+', glyph)) == 16 for glyph in glyphs)
font = '#ifndef OLED_FONT_H\n#define OLED_FONT_H\n\n#include <stdint.h>\n\n'
font += '/* 字形沿用原 uart 工程的 asc2_1608；上半页 8 列，然后是下半页 8 列。 */\n'
font += 'static const uint8_t oled_font_8x16[95][16] = {\n'
for i, glyph in enumerate(glyphs):
    values = re.findall(r'0x[0-9a-fA-F]+', glyph)
    font += '    {' + ', '.join(values) + '}, /* ASCII ' + str(i + 32) + ' */\n'
font += '};\n\n#endif\n'
(STAGE / 'Modules/OLED/oled_font.h').write_text(font, encoding='utf-8')

main_path = STAGE / 'Core/Src/main.c'
main = main_path.read_text(encoding='utf-8')
replacements = {
    '/* USER CODE BEGIN Includes */\n': '/* USER CODE BEGIN Includes */\n#include "app_init.h"\n',
    '/* USER CODE BEGIN 2 */\n': '/* USER CODE BEGIN 2 */\n  /* 无复位引脚的 OLED 先等待上电稳定，再注册和初始化应用。 */\n  HAL_Delay(200U);\n  if (!App_Init())\n  {\n    Error_Handler();\n  }\n',
    '/* USER CODE BEGIN 3 */\n': '/* USER CODE BEGIN 3 */\n    App_Run(HAL_GetTick());\n    HAL_Delay(1U);\n',
}
for old, new in replacements.items():
    assert main.count(old) == 1, old
    main = main.replace(old, new)
main_path.write_text(main, encoding='utf-8')

cmake_path = STAGE / 'CMakeLists.txt'
cmake = cmake_path.read_text(encoding='utf-8')
assert cmake.count('    # Add user sources here') == 1
cmake = cmake.replace('    # Add user sources here', '\n'.join([
    '    APP/app_init.c', '    APP/app_oled.c',
    '    Bsp/iic/bsp_iic.c', '    Modules/OLED/oled.c',
]))
assert cmake.count('    # Add user defined include paths') == 1
cmake = cmake.replace('    # Add user defined include paths', '\n'.join([
    '    APP', '    Bsp/iic', '    Modules/OLED',
]))
cmake += '\n# 独立例程明确列出源文件；不递归编译其它例程。\n'
cmake += 'set_source_files_properties(\n    APP/app_init.c APP/app_oled.c\n'
cmake += '    Bsp/iic/bsp_iic.c Modules/OLED/oled.c\n'
cmake += '    PROPERTIES COMPILE_OPTIONS "-Wextra;-Werror"\n)\n'
cmake += '\n# 编译后生成可用于烧录的 BIN 和 HEX，工具由工具链配置提供。\n'
cmake += 'add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD\n'
cmake += '    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"\n'
cmake += '    COMMAND ${CMAKE_OBJCOPY} -O ihex "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex"\n'
cmake += '    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex"\n'
cmake += '    VERBATIM\n)\n'
cmake_path.write_text(cmake, encoding='utf-8')

files = [
    'Core/Src/main.c', 'CMakeLists.txt', '.gitignore', 'README.md',
    'APP/app_init.c', 'APP/app_init.h', 'APP/app_oled.c', 'APP/app_oled.h',
    'Bsp/iic/bsp_iic.c', 'Bsp/iic/bsp_iic.h',
    'Modules/OLED/oled.c', 'Modules/OLED/oled.h', 'Modules/OLED/oled_font.h',
    'Bsp/iic/README.md', 'Modules/OLED/README.md', 'docs/rm_framework_secondary_development.md',
]
manifest = []
for relative in files:
    original = TARGET / relative
    prepared = STAGE / relative
    assert prepared.is_file(), relative
    manifest.append({
        'path': relative,
        'original_sha256': hashlib.sha256(original.read_bytes()).hexdigest() if original.exists() else None,
        'prepared_sha256': hashlib.sha256(prepared.read_bytes()).hexdigest(),
    })
(STAGE.parent / 'install_manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print(f'Prepared {len(files)} files; original target has not been modified.')
