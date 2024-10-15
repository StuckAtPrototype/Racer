import asyncio
import os
import json
from bleak import BleakClient, BleakScanner
import keyboard

CONFIG_FILE = "ble_device_config.json"
CHARACTERISTIC_UUID = "23408888-1f40-4cd8-9b89-ca8d45f8a5b0"  # Replace with your characteristic UUID


async def send_command(client, speedA, speedB, directionA, directionB, duration):
    command = bytearray([speedA, speedB, directionA, directionB, duration])
    await client.write_gatt_char(CHARACTERISTIC_UUID, command)

async def select_device():
    devices = await BleakScanner.discover()
    if not devices:
        print("No BLE devices found.")
        return None

    print("Available BLE devices:")
    for i, device in enumerate(devices):
        print(f"{i}: {device.name} - {device.address}")

    while True:
        try:
            selection = int(input("Select device by number: "))
            if 0 <= selection < len(devices):
                return devices[selection].address
            else:
                print("Invalid selection. Please try again.")
        except ValueError:
            print("Please enter a valid number.")

def load_config():
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            return json.load(f)
    return {}

def save_config(config):
    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f)

async def get_ble_address():
    config = load_config()
    saved_address = config.get("ble_address")

    if saved_address:
        print(f"Previously connected to device with address: {saved_address}")
        choice = input("Do you want to connect to the same device? (y/n): ").lower()
        if choice == 'y':
            return saved_address

    new_address = await select_device()
    if new_address:
        config["ble_address"] = new_address
        save_config(config)
    return new_address

async def handle_key_press(client):

    # motor A speed, motor A dir, motor B speed, motor B dir, time 50 == 5.0 seconds
    if keyboard.is_pressed('w'):
        await send_command(client, 60,1,60,1,5)
        print(f"Forward")
    elif keyboard.is_pressed('s'):
        await send_command(client, 50,0,50,0,5)
        print(f"Backwards")
    elif keyboard.is_pressed('d'):
        await send_command(client, 40,1,40,0,3)
        print(f"Right")
    elif keyboard.is_pressed('a'):
        await send_command(client, 40,0,40,1,3)
        print(f"Left")



async def main():
    global motor_speed

    ble_address = await get_ble_address()
    if not ble_address:
        print("No device selected. Exiting.")
        return

    print(f"Connecting to device: {ble_address}")
    async with BleakClient(ble_address) as client:
        print(f"Connected: {await client.is_connected()}")
        print("Press 'w' to speed up, 's' to slow down, or 'q' to quit.")

        while True:
            await handle_key_press(client)
            if keyboard.is_pressed('q'):
                print("Quitting...")
                break

if __name__ == "__main__":
    asyncio.run(main())
