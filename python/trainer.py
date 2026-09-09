import serial
import time


# ============================================================
# CONFIGURATION
# ============================================================

PORT = "/dev/ttyACM0"
BAUDRATE = 115200


APPLICATIONS = {
    "0": "STOP",
    "1": "LED",
    "2": "BUTTON",
    "3": "RELAY",
    "4": "TOUCH",
    "5": "MOTOR",
    "6": "PIR",
    "7": "7 SEGMENT",
    "8": "JOYSTICK",
    "9": "SERVO",
    "10": "POTENTIOMETER",
    "11": "IO EXPANDER",
    "12": "PROXIMITY",
    "13": "IMU",
    "14": "TEMPERATURE",
    "15": "OLED",
    "16": "SECURE",
    "17": "EEPROM",
    "18": "SD CARD",
    "19": "WIFI"
}

# ============================================================
# CONNECT TO PSoC
# ============================================================

def connect():

    print("Connecting to PSoC Trainer Kit...")

    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        timeout=0.1
    )

    # Opening the serial port may reset the PSoC.
    time.sleep(2)

    # Remove startup messages already waiting in the RX buffer.
    ser.reset_input_buffer()

    print("Connected.")

    return ser


# ============================================================
# SEND COMMAND
# ============================================================

def send_command(ser, command):

    # Clear anything left from the previous transaction.
    ser.reset_input_buffer()

    # Send command + newline.
    ser.write((command + "\n").encode())

    # Make sure data has physically left the PC.
    ser.flush()

    # Collect PSoC response.
    response_lines = []

    start_time = time.time()

    while time.time() - start_time < 1.0:

        line = ser.readline()

        if line:

            text = line.decode(
                errors="ignore"
            ).strip()

            if text:

                response_lines.append(text)

                # Stop after receiving the final response.
                if text.startswith("OK:") or text.startswith("ERR:"):
                    break

    # Display response.
    for line in response_lines:
        print("PSoC:", line)


# ============================================================
# DISPLAY APPLICATION LIST
# ============================================================

def show_menu(): 
    print() 
    print("============================================================") 
    print(" PSoC TRAINER KIT CONTROL") 
    print("============================================================") 
    print(" 0 - STOP         1 - LED               2 - BUTTON") 
    print(" 3 - RELAY        4 - TOUCH             5 - MOTOR") 
    print(" 6 - PIR          7 - 7 SEGMENT         8 - JOYSTICK") 
    print(" 9 - SERVO        10 - POTENTIOMETER    11 - IO EXPANDER") 
    print("12 - PROXIMITY    13 - IMU              14 - TEMPERATURE") 
    print("15 - OLED         16 - SECURE           17 - EEPROM") 
    print("18 - SD CARD      19 - WIFI") 
    print("------------------------------------------------------------") 
    print("ls - LIST APPLICATIONS                  q - EXIT") 
    print("============================================================")


# ============================================================
# MAIN
# ============================================================

def main():

    ser = None

    try:

        ser = connect()

        show_menu()

        while True:

            command = input("\nEnter command: ").strip()

            # ------------------------------------------------
            # EXIT
            # ------------------------------------------------

            if command.lower() == "q":
                break


            # ------------------------------------------------
            # LIST APPLICATIONS
            # ------------------------------------------------

            if command.lower() == "ls":
                show_menu()
                continue


            # ------------------------------------------------
            # APPLICATION COMMAND
            # ------------------------------------------------

            if command in APPLICATIONS:

                print(
                    f"Starting command {command}: "
                    f"{APPLICATIONS[command]}"
                )

                send_command(
                    ser,
                    command
                )

            else:

                print(
                    "Invalid command. "
                    "Use 0-19, ls, or q."
                )


    except KeyboardInterrupt:

        print("\nProgram interrupted.")


    except serial.SerialException as error:

        print(f"\nSerial error: {error}")


    finally:

        if ser is not None and ser.is_open:
            ser.close()

        print("Disconnected.")


# ============================================================
# ENTRY POINT
# ============================================================

if __name__ == "__main__":
    main()