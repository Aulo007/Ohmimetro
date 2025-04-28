import re

def convert_to_font_pattern(pattern):
    entries = [entry.strip() for entry in pattern.split(",") if entry.strip()]
    if len(entries) != 64:
        raise ValueError("Padrão inválido. Esperado 64 entradas (8x8).")
    
    rows = [entries[i * 8 : (i + 1) * 8] for i in range(8)]
    bytes_array = []
    
    for x in range(8):
        byte = 0
        for y in range(8):
            # Verifica se o pixel está ativo (qualquer valor diferente de 0xff000000)
            if rows[y][x] != "0xff000000":
                byte |= 1 << y
        bytes_array.append(byte)
    
    hex_values = [f"0x{byte:02x}" for byte in bytes_array]
    return ", ".join(hex_values)

def convert_multiple_font_patterns(input_text):
    blocks = re.findall(r"\{([^}]+)\}", input_text)
    results = []
    for block in blocks:
        cleaned_block = block.replace("\n", " ").strip()
        result = convert_to_font_pattern(cleaned_block)
        results.append(result)
    return results

# Entrada com múltiplos padrões
pattern_input = """
{
0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 
0xff00ff00, 0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 
0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 
0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 
0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 
0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 
0xff000000, 0xff00ff00, 0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000, 0xff00ff00, 
0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff00ff00, 0xff000000
}
"""

# Processa e imprime no formato desejado
results = convert_multiple_font_patterns(pattern_input)
print("Padrões convertidos para font.h:")
for result in results:
    print(f"{result},")