import numpy as np
import tensorflow as tf

# Charger le modèle TFLite
model_path = 'weather_model/model.tflite'
with open(model_path, 'rb') as f:
    model_content = f.read()

# Convertir le modèle en format C++
header_file_path = 'weather_model/model.h'
with open(header_file_path, 'w') as f:
    f.write('#ifndef MODEL_H\n')
    f.write('#define MODEL_H\n\n')
    f.write('const unsigned char g_model[] = {\n')
    
    # Écrire le modèle en format C++
    for i in range(0, len(model_content), 16):
        chunk = model_content[i:i+16]
        f.write('    ')
        f.write(', '.join(f'0x{b:02x}' for b in chunk))
        f.write(',\n')
    
    f.write('};\n\n')
    f.write('#endif  // MODEL_H\n')
