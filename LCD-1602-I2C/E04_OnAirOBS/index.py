import asyncio
import serial
import serial_asyncio
import sys
import signal
import platform
from obswebsocket import obsws, requests

class OBSRecordingController:
    def __init__(self):
        self.obs = None
        self.serial_port = None
        self.serial_reader = None
        self.serial_writer = None
        self.is_recording = False
        self.initial_synced = False
        self.obs_connected = False
        self.running = True
        
        # OBS WebSocket configuration
        self.obs_host = "192.168.1.7"
        self.obs_port = 4455
        self.obs_password = ""  # Add password if needed
        
        # Serial port configuration
        self.baud_rate = 9600
        self.serial_port_name = self.find_serial_port()
        
        # Setup signal handlers
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
    
    def find_serial_port(self):
        """Find the appropriate serial port based on platform"""
        if platform.system() == "Windows":
            return "COM7"
        elif platform.system() == "Darwin":  # macOS
            return "/dev/tty.usbmodem101"
        else:  # Linux and others
            return "/dev/ttyUSB0"
    
    def signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        print("\nShutting down...")
        self.running = False
        if self.serial_writer:
            self.serial_writer.close()
        if self.obs:
            self.obs.disconnect()
        sys.exit(0)
    
    def send_to_serial(self, line):
        """Send data to serial port"""
        if not self.serial_writer:
            print("Serial writer is None")
            return
        
        try:
            print(f"ATTEMPTING WRITE: {line}")
            self.serial_writer.write((line + "\n").encode())
            print(f"WRITE SUCCESS: {line}")
        except Exception as e:
            print(f"WRITE ERROR: {e}")
    
    def send_rgb(self, group, r, g, b):
        """Send RGB status to serial"""
        self.send_to_serial(f"{group}{1 if r else 0}{1 if g else 0}{1 if b else 0}")
    
    async def initial_sync_sequence(self):
        """Initial synchronization with OBS"""
        if not self.serial_writer or not self.obs_connected or self.initial_synced:
            return
        
        try:
            # Send test signal
            self.send_to_serial("TEST")
            
            # Get recording status from OBS
            response = await self.obs.call(requests.GetRecordStatus())
            self.is_recording = response.outputActive
            
            # Update LED
            self.send_rgb("r", 1 if response.outputActive else 0, 0, 0)
            print(f"Initial recording status: {'ON AIR' if response.outputActive else 'OFF'}")
            self.initial_synced = True
            
        except Exception as e:
            print(f"Initial sync error: {e}")
            self.initial_synced = True
    
    async def connect_serial(self):
        """Connect to serial port"""
        try:
            # Close existing connection if any
            if self.serial_writer:
                self.serial_writer.close()
                await asyncio.sleep(0.1)
            
            print(f"Connecting to serial port: {self.serial_port_name}")
            
            # Open serial connection
            self.serial_reader, self.serial_writer = await serial_asyncio.open_serial_connection(
                url=self.serial_port_name,
                baudrate=self.baud_rate
            )
            
            print(f"Serial port opened: {self.serial_port_name}")
            
            # Start reading from serial port
            asyncio.create_task(self.read_serial_data())
            
            # Wait a moment and sync
            await asyncio.sleep(0.12)
            await self.initial_sync_sequence()
            
        except Exception as e:
            print(f"Serial port error: {e}")
            # Retry after 5 seconds
            if self.running:
                await asyncio.sleep(5)
                asyncio.create_task(self.connect_serial())
    
    async def read_serial_data(self):
        """Read and process serial data"""
        try:
            while self.running and self.serial_reader:
                try:
                    # Read a line from serial
                    line = await self.serial_reader.readline()
                    line = line.decode().strip()
                    
                    if line:
                        print(f"Arduino: {line}")
                        
                        if not self.initial_synced:
                            continue
                        
                        # Handle button presses
                        if line == "BTN-RECORD":
                            await self.obs.call(requests.ToggleRecord())
                            
                except Exception as e:
                    print(f"Serial read error: {e}")
                    break
                    
        except Exception as e:
            print(f"Serial read loop error: {e}")
        
        print("Serial read loop ended")
    
    async def connect_obs(self):
        """Connect to OBS WebSocket"""
        try:
            print(f"Connecting to OBS at ws://{self.obs_host}:{self.obs_port}")
            
            # Create OBS WebSocket connection
            self.obs = obsws(self.obs_host, self.obs_port, self.obs_password)
            await self.obs.connect()
            self.obs_connected = True
            
            print("OBS connected")
            
            # Get initial recording status
            response = await self.obs.call(requests.GetRecordStatus())
            self.is_recording = response.outputActive
            
            # Initial sync
            await self.initial_sync_sequence()
            print(f"Recording status: {'ON AIR' if response.outputActive else 'OFF'}")
            
            # Register event handlers
            self.obs.register(self.on_record_state_changed)
            self.obs.register(self.on_connection_closed)
            
        except Exception as e:
            print(f"OBS connection error: {e}")
            self.obs_connected = False
            self.initial_synced = False
            
            # Retry after 5 seconds
            if self.running:
                await asyncio.sleep(5)
                print("Reconnecting to OBS...")
                asyncio.create_task(self.connect_obs())
    
    def on_record_state_changed(self, data):
        """Handle OBS record state changes"""
        try:
            self.is_recording = data.outputActive
            self.send_rgb("r", 1 if data.outputActive else 0, 0, 0)
            print(f"Recording: {'ON AIR' if data.outputActive else 'OFF'}")
            
            if data.outputActive:
                self.send_to_serial("ON")
            else:
                self.send_to_serial("OFF")
                
        except Exception as e:
            print(f"Record state handler error: {e}")
    
    def on_connection_closed(self, data):
        """Handle OBS connection closed"""
        print("OBS connection closed")
        self.obs_connected = False
        self.initial_synced = False
        
        if self.running:
            asyncio.create_task(self.reconnect_obs())
    
    async def reconnect_obs(self):
        """Reconnect to OBS after disconnection"""
        await asyncio.sleep(5)
        print("Reconnecting to OBS...")
        await self.connect_obs()
    
    async def run(self):
        """Main application loop"""
        print("Starting OBS Recording Controller")
        print(f"OBS target: ws://{self.obs_host}:{self.obs_port}")
        print(f"Serial port: {self.serial_port_name}")
        
        # Start connections
        await asyncio.gather(
            self.connect_serial(),
            self.connect_obs(),
            return_exceptions=True
        )
        
        # Keep running
        while self.running:
            await asyncio.sleep(1)

async def main():
    controller = OBSRecordingController()
    await controller.run()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nShutting down...")
        sys.exit(0)