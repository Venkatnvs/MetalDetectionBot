import React, { useState, useEffect } from 'react';
import {
  ArrowUp,
  ArrowDown,
  ArrowLeft,
  ArrowRight,
  Wifi,
  AlertTriangle,
  Square
} from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Slider } from '@/components/ui/slider';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';
import { ref, set, onValue } from 'firebase/database';
import { database } from '@/firebase/firebaseConfig';

const LiveControlPage = () => {
  const [status, setStatus] = useState('');
  const [showStatusMessage, setShowStatusMessage] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [speed, setSpeed] = useState(128);

  const handleSpeedChange = async (value) => {
    if (connectionStatus !== 'connected') {
      setStatus('Connection error. Please check your connection.');
      return;
    }
    setSpeed(value);
    try {
      const speedRef = ref(database, 'metal-detection-bot/triggers');
      await set(speedRef, {
        command: 'speed',
        speed: value,
        timestamp: Date.now()
      });
    } catch (error) {
      setStatus('Error setting speed');
      console.error(error);
    }
  };

  useEffect(() => {
    if (status) {
      setShowStatusMessage(true);
      const timer = setTimeout(() => {
        setShowStatusMessage(false);
      }, 3000);
      return () => clearTimeout(timer);
    }
  }, [status]);

  useEffect(() => {
    const connectedRef = ref(database, '.info/connected');
    const unsubscribe = onValue(connectedRef, (snapshot) => {
      if (snapshot.val() === true) {
        setConnectionStatus('connected');
      } else {
        setConnectionStatus('error');
      }
    });
    sendCommand('S');
    return () => unsubscribe();
  }, []);

  const sendCommand = async (command) => {
    try {
      const commandRef = ref(database, 'metal-detection-bot/triggers');
      await set(commandRef, {
        command: command,
        speed: speed,
        timestamp: Date.now(),
      });
    } catch (error) {
      setStatus('Error sending command');
      console.error(error);
    }
  };

  const JoystickControl = () => {
    const [activeDirection, setActiveDirection] = useState(null);
    const [stickPosition, setStickPosition] = useState({ x: 0, y: 0 });
    const containerSize = 200;
    const stickSize = 50;
    const maxDistance = (containerSize - stickSize) / 2;

    const handlePointerDown = (e) => {
      if (connectionStatus !== 'connected') {
        setStatus('Connection error. Please check your connection.');
        return;
      }
      const containerRect = e.currentTarget.getBoundingClientRect();
      handleMove(e.clientX, e.clientY, containerRect);
    };

    const handlePointerMove = (e) => {
      if (activeDirection === null) return;
      const containerRect = e.currentTarget.getBoundingClientRect();
      handleMove(e.clientX, e.clientY, containerRect);
    };

    const handleMove = (clientX, clientY, containerRect) => {
      const centerX = containerRect.left + containerSize / 2;
      const centerY = containerRect.top + containerSize / 2;

      const deltaX = clientX - centerX;
      const deltaY = clientY - centerY;

      const distance = Math.min(Math.sqrt(deltaX * deltaX + deltaY * deltaY), maxDistance);
      const angle = Math.atan2(deltaY, deltaX);

      const stickX = distance * Math.cos(angle);
      const stickY = distance * Math.sin(angle);

      setStickPosition({ x: stickX, y: stickY });

      let direction = 'S';
      // Only 4 directions: F (up), B (down), R (right), L (left)
      if (distance > maxDistance * 0.2) {
        const degrees = angle * (180 / Math.PI);
        const adjustedDegrees = degrees < 0 ? degrees + 360 : degrees;
        if (adjustedDegrees >= 45 && adjustedDegrees < 135) direction = 'B'; // Up
        else if (adjustedDegrees >= 135 && adjustedDegrees < 225) direction = 'L'; // Left
        else if (adjustedDegrees >= 225 && adjustedDegrees < 315) direction = 'F'; // Down
        else direction = 'R'; // Right
      }

      if (direction !== activeDirection) {
        setActiveDirection(direction);
        sendCommand(direction);
      }
    };

    const handlePointerUp = () => {
      if (activeDirection !== null) {
        setActiveDirection(null);
        setStickPosition({ x: 0, y: 0 });
        sendCommand('S');
      }
    };

    return (
      <div
        className="relative bg-gray-200 rounded-full flex items-center justify-center"
        style={{ width: containerSize, height: containerSize }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerLeave={handlePointerUp}
      >
        <div
          className="absolute bg-primary rounded-full shadow-md cursor-grab"
          style={{
            width: stickSize,
            height: stickSize,
            transform: `translate(${stickPosition.x}px, ${stickPosition.y}px)`,
            touchAction: 'none',
          }}
        ></div>
        {/* Direction indicators */}
        <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
            {/* Center circle */}
            <div className={`w-8 h-8 rounded-full flex items-center justify-center ${activeDirection ? 'bg-red-500' : 'bg-gray-200'} transition-colors duration-200`}>
                <Square size={16} className="text-white" />
            </div>

            {/* 4 Direction Arrows */}
            {/* Up (F) */}
            <div className="absolute top-2 left-1/2 transform -translate-x-1/2 text-primary opacity-50"><ArrowUp size={24} /></div>
            {/* Right (R) */}
            <div className="absolute right-2 top-1/2 transform -translate-y-1/2 text-primary opacity-50"><ArrowRight size={24} /></div>
            {/* Down (B) */}
            <div className="absolute bottom-2 left-1/2 transform -translate-x-1/2 text-primary opacity-50"><ArrowDown size={24} /></div>
            {/* Left (L) */}
            <div className="absolute left-2 top-1/2 transform -translate-y-1/2 text-primary opacity-50"><ArrowLeft size={24} /></div>
        </div>
      </div>
    );
  };

  const ConnectionIndicator = () => {
    const statusConfig = {
      connected: {
        variant: "outline",
        className: "bg-green-50 text-green-700 border-green-200",
        label: "Connected",
        icon: <Wifi size={16} className="mr-1" />
      },
      disconnected: {
        variant: "outline",
        className: "bg-red-50 text-red-700 border-red-200",
        label: "Disconnected",
        icon: <AlertTriangle size={16} className="mr-1" />
      },
      error: {
        variant: "outline",
        className: "bg-amber-50 text-amber-700 border-amber-200",
        label: "Connection Error",
        icon: <AlertTriangle size={16} className="mr-1" />
      },
      connecting: {
        variant: "outline",
        className: "bg-blue-50 text-blue-700 border-blue-200",
        label: "Connecting...",
        icon: <Wifi size={16} className="mr-1 animate-pulse" />
      }
    };
    
    const config = statusConfig[connectionStatus];
    
    return (
      <Badge 
        variant={config.variant}
        className={config.className}
      >
        {config.icon}
        {config.label}
      </Badge>
    );
  };

  const SpeedControl = () => {
    const presets = [
      { name: 'Slow', value: 64 },
      { name: 'Medium', value: 128 },
      { name: 'Fast', value: 192 },
      { name: 'Max', value: 255 }
    ];

    return (
      <div className="space-y-3 w-full mt-3">
        <div className="flex justify-between items-center">
          <h3 className="text-sm font-medium">Speed</h3>
          <Badge variant="secondary" className="font-medium">
            {speed}
          </Badge>
        </div>
        
        <Slider
          value={[speed]}
          onValueChange={(value) => {
            handleSpeedChange(value[0]);
          }}
          max={255}
          step={10}
          className="w-full"
        />
        
        <div className="flex justify-between text-xs text-muted-foreground px-1">
          <span>Min</span>
          <span>Mid</span>
          <span>Max</span>
        </div>
        
        <div className="flex gap-2 mt-2">
          {presets.map((preset) => (
            <Button
              key={preset.name}
              onClick={() => {
                handleSpeedChange(preset.value);
              }}
              variant={speed === preset.value ? "default" : "outline"}
              className="flex-1 h-8"
              size="sm"
            >
              {preset.name}
            </Button>
          ))}
        </div>
      </div>
    );
  };

  return (
    <div className="py-2">
      <div className="max-w-7xl mx-auto px-2 py-0 space-y-6">
        <div className="flex justify-end items-center">
          <ConnectionIndicator />
        </div>
        {showStatusMessage && status && (
          <Alert variant="destructive">
            <AlertTriangle className="h-4 w-4" />
            <AlertTitle>Alert</AlertTitle>
            <AlertDescription>{status}</AlertDescription>
          </Alert>
        )}

        <div className="grid grid-cols-1 gap-6">
          <Card>
            <CardContent className="p-6">
              <div>
                {/* Container for all controls to enable flex row layout and centering */}
                <div className="flex flex-col items-center justify-center">
                  {/* Main Movement and Rotation Controls */}
                  <div className="flex items-center justify-center space-x-4 mb-4 sm:mb-0 sm:mr-10">
                    {/* Joystick Control (Center) */}
                    <JoystickControl />
                  </div>
                  <Separator orientation="vertical" className="h-full my-5" />
                  <div className="flex items-center justify-center">
                    <SpeedControl />
                  </div>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  );
};

export default LiveControlPage;