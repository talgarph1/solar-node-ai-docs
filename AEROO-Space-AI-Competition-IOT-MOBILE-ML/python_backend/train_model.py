import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestRegressor
from sklearn.preprocessing import StandardScaler
import pickle
import os

# NASA dataset yolu
DATASET_PATH = r"C:\Users\Suleyman\Desktop\ai proje yarışması\nasa dataset\cleaned_dataset"

def load_and_prepare_data():
    """NASA dataset'ini yükle ve işle"""
    print(f"Dataset yolu: {DATASET_PATH}")

    metadata_file = os.path.join(DATASET_PATH, "metadata.csv")
    print(f"Metadata dosyası: {metadata_file}")

    if not os.path.exists(metadata_file):
        print(f"HATA: Metadata dosyası bulunamadı: {metadata_file}")
        return pd.DataFrame()

    metadata = pd.read_csv(metadata_file)
    print(f"Metadata yüklendi. Toplam {len(metadata)} kayıt.")

    # Capacity sütununu sayısal değere çevir
    metadata['Capacity'] = pd.to_numeric(metadata['Capacity'], errors='coerce')

    # Sadece discharge testlerini al (kapasiteyi ölçenler)
    discharge_tests = metadata[metadata['type'] == 'discharge'].copy()
    print(f"Discharge testleri: {len(discharge_tests)} kayıt")

    # NaN olmayan capacity değerleri olan satırları al
    discharge_tests = discharge_tests[discharge_tests['Capacity'].notna()]
    print(f"Capacity değeri olan testler: {len(discharge_tests)} kayıt")

    # Her batarya için capacity değişimini hesapla
    battery_data = []
    error_count = 0
    success_count = 0

    for battery_id in discharge_tests['battery_id'].unique()[:5]:  # İlk 5 batarya ile test et
        battery_tests = discharge_tests[discharge_tests['battery_id'] == battery_id].sort_values('test_id')

        print(f"\nBatarya {battery_id}: {len(battery_tests)} test")

        if len(battery_tests) < 2:
            continue

        initial_capacity = battery_tests.iloc[0]['Capacity']

        for idx, row in battery_tests.iterrows():
            data_file = os.path.join(DATASET_PATH, 'data', row['filename'])

            try:
                cycle_data = pd.read_csv(data_file)

                # Basit istatistiksel özellikler
                avg_temp = cycle_data['Temperature_measured'].mean()
                avg_voltage = cycle_data['Voltage_measured'].mean()
                avg_current = cycle_data['Current_measured'].mean()

                # Capacity degradation (batarya yaşlanması)
                current_capacity = row['Capacity']
                capacity_fade = (initial_capacity - current_capacity) / initial_capacity * 100

                # Remaining useful life (kalan ömür yüzdesi)
                rul_percent = (current_capacity / initial_capacity) * 100

                battery_data.append({
                    'Temperature': avg_temp,
                    'Voltage': avg_voltage,
                    'Current': abs(avg_current),
                    'Cycle_Count': int(row['test_id']),
                    'Capacity_Fade': capacity_fade,
                    'RUL_Percent': rul_percent
                })
                success_count += 1
            except Exception as e:
                error_count += 1
                if error_count < 5:
                    print(f"  Hata ({row['filename']}): {e}")

    print(f"\nBaşarılı: {success_count}, Hatalı: {error_count}")
    return pd.DataFrame(battery_data)

def train_model():
    """Model eğit ve kaydet"""
    print("Veri yükleniyor...")
    df = load_and_prepare_data()

    print(f"Toplam {len(df)} örnek yüklendi")
    print(df.head())

    # Feature'lar ve target
    features = ['Temperature', 'Voltage', 'Current', 'Cycle_Count', 'Capacity_Fade']
    X = df[features]
    y = df['RUL_Percent']

    # Normalizasyon
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    # Model eğitimi
    print("\nModel eğitiliyor...")
    model = RandomForestRegressor(n_estimators=100, random_state=42, max_depth=10)
    model.fit(X_scaled, y)

    print(f"Model eğitimi tamamlandı. R² Score: {model.score(X_scaled, y):.3f}")

    # Model ve scaler'ı kaydet
    with open('battery_model.pkl', 'wb') as f:
        pickle.dump(model, f)

    with open('scaler.pkl', 'wb') as f:
        pickle.dump(scaler, f)

    print("\nModel kaydedildi: battery_model.pkl")
    print("Scaler kaydedildi: scaler.pkl")

    # Örnek tahmin
    print("\n--- Örnek Tahmin ---")
    sample = [[25, 3.8, 1.0, 50, 10]]  # Temperature, Voltage, Current, Cycle, Capacity_Fade
    sample_scaled = scaler.transform(sample)
    prediction = model.predict(sample_scaled)
    print(f"Girdi: Temp=25°C, Voltage=3.8V, Current=1.0A, Cycle=50, Capacity_Fade=10%")
    print(f"Tahmin: Batarya Sağlığı = {prediction[0]:.1f}%")

if __name__ == "__main__":
    train_model()
