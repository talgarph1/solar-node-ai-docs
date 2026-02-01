import json
import time
import random

vision_file = r'C:\Users\Suleyman\Desktop\STM32 training\STM32N6-GettingStarted-ObjectDetection-main\vision_data.txt'

def generate_anomaly():
    """Generate random anomaly object"""
    anomaly_type = random.choice(['dust', 'dust', 'dust', 'cover'])  # 75% dust, 25% cover

    obj = {
        'c': 2 if anomaly_type == 'dust' else 0,
        'label': anomaly_type,
        'p': round(random.uniform(0.3, 0.95), 3),
        'x': round(random.uniform(0.1, 0.9), 3),
        'y': round(random.uniform(0.1, 0.9), 3),
        'w': round(random.uniform(0.1, 0.4), 3),
        'h': round(random.uniform(0.1, 0.4), 3)
    }
    return obj

def simulate_vision_data():
    """Simulate STM32N6 vision data at 50ms intervals"""
    print("Vision data simulator started...")
    print(f"Writing to: {vision_file}")
    print("Press Ctrl+C to stop\n")

    timestamp_ms = int(time.time() * 1000) % 1000000  # Start from current time

    try:
        while True:
            # Generate 0-4 anomalies
            num_objects = random.randint(0, 4)
            objs = [generate_anomaly() for _ in range(num_objects)]

            # Create vision data JSON
            vision_data = {
                't': timestamp_ms,
                'n': num_objects,
                'inf_ms': random.randint(30, 35),
                'objs': objs
            }

            # Write to file
            with open(vision_file, 'a') as f:
                f.write(json.dumps(vision_data) + '\n')

            # Print summary
            dust_count = sum(1 for o in objs if o['label'] == 'dust')
            cover_count = sum(1 for o in objs if o['label'] == 'cover')
            print(f"[t={timestamp_ms:6d}] Objects: {num_objects} (Dust: {dust_count}, Cover: {cover_count})")

            timestamp_ms += 50  # Increment by 50ms
            time.sleep(0.05)  # 50ms delay

    except KeyboardInterrupt:
        print("\n\nSimulation stopped.")

if __name__ == '__main__':
    simulate_vision_data()
