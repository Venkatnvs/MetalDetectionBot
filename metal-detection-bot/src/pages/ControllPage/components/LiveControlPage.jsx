import React, { useState, useEffect } from 'react';
import {
  ArrowUp,
  ArrowDown,
  ArrowLeft,
  ArrowRight,
  Wifi,
  AlertTriangle,
  Square,
  Check,
  XCircle,
  MapPin,
  ExternalLink
} from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';
import { Button } from '@/components/ui/button';
import { ref, set, onValue, remove } from 'firebase/database';
import { database } from '@/firebase/firebaseConfig';

const LiveControlPage = () => {
  const [status, setStatus] = useState('');
  const [showStatusMessage, setShowStatusMessage] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const [metalDetected, setMetalDetected] = useState(null);
  const [gpsLocation, setGpsLocation] = useState(null);
  const [gpsStatus, setGpsStatus] = useState({ hasFix: false, satellites: 0, hdop: 99.9 });

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
    const unsubscribeConnection = onValue(connectedRef, (snapshot) => {
      if (snapshot.val() === true) {
        setConnectionStatus('connected');
      } else {
        setConnectionStatus('error');
      }
    });
    sendCommand('S');
    return () => unsubscribeConnection();
  }, []);

  useEffect(() => {
    const detectionRef = ref(database, 'metal-detection-bot-2/detection');
    setMetalDetected(null);
    const setInitialValue = async () => {
      await remove(detectionRef);
    };
    setInitialValue();
    const unsubscribeDetection = onValue(detectionRef, (snapshot) => {
      setMetalDetected(snapshot.val());
    });
    return () => unsubscribeDetection();
  }, []);

  useEffect(() => {
    const locationRef = ref(database, 'metal-detection-bot-2/location');
    const unsubscribeLocation = onValue(locationRef, (snapshot) => {
      const locationData = snapshot.val();
      if (locationData && locationData.latitude && locationData.longitude) {
        setGpsLocation({
          latitude: locationData.latitude,
          longitude: locationData.longitude,
          timestamp: locationData.timestamp || Date.now(),
          satellites: locationData.satellites || 0,
          hdop: locationData.hdop || 99.9,
          hasFix: locationData.hasFix !== undefined ? locationData.hasFix : true
        });
      }
    });
    return () => unsubscribeLocation();
  }, []);

  useEffect(() => {
    const gpsStatusRef = ref(database, 'metal-detection-bot-2/gpsStatus');
    const unsubscribeGpsStatus = onValue(gpsStatusRef, (snapshot) => {
      const statusData = snapshot.val();
      if (statusData) {
        setGpsStatus({
          hasFix: statusData.hasFix || false,
          satellites: statusData.satellites || 0,
          hdop: statusData.hdop || 99.9
        });
      }
    });
    return () => unsubscribeGpsStatus();
  }, []);

  const sendCommand = async (command) => {
    try {
      const commandRef = ref(database, 'metal-detection-bot-2/triggers');
      await set(commandRef, {
        command: command,
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
            <CardHeader>
              <CardTitle>Live Control & Metal Detection</CardTitle>
            </CardHeader>
            <CardContent className="p-2">
              <div>
                {/* Container for all controls to enable flex row layout and centering */}
                <div className="flex flex-col items-center justify-center">
                  {/* Main Movement and Rotation Controls */}
                  <div className="flex items-center justify-center space-x-4 mb-4 sm:mb-0 sm:mr-10">
                    {/* Joystick Control (Center) */}
                    <JoystickControl />
                  </div>
                  <Separator orientation="horizontal" className="h-full my-5" />
                  <div className="flex flex-col items-center justify-center p-4 space-y-4 w-full">
                    {metalDetected === null ? (
                      <Badge variant="outline" className="bg-gray-100 text-gray-700 border-gray-200 text-lg p-3">
                        <AlertTriangle size={24} className="mr-2" />
                        Metal Detection: Unknown
                      </Badge>
                    ) : metalDetected ? (
                      <Badge variant="outline" className="bg-green-100 text-green-700 border-green-200 text-lg p-3">
                        <Check size={24} className="mr-2" />
                        Metal Detected
                      </Badge>
                    ) : (
                      <Badge variant="outline" className="bg-red-100 text-red-700 border-red-200 text-lg p-3">
                        <XCircle size={24} className="mr-2" />
                        No Metal Detected
                      </Badge>
                    )}
                    
                    {/* GPS Location Display - Always show latest location or status */}
                    <div className="w-full max-w-2xl space-y-4">
                      <Card>
                        <CardHeader>
                          <div className="flex items-center justify-between">
                            <CardTitle className="flex items-center gap-2">
                              <MapPin className="h-5 w-5" />
                              GPS Location
                            </CardTitle>
                            {/* GPS Status Badge */}
                            {gpsStatus.hasFix ? (
                              <Badge variant="outline" className="bg-green-100 text-green-700 border-green-200">
                                <Check size={14} className="mr-1" />
                                GPS Fix ({gpsStatus.satellites} sats)
                              </Badge>
                            ) : (
                              <Badge variant="outline" className="bg-yellow-100 text-yellow-700 border-yellow-200">
                                <AlertTriangle size={14} className="mr-1" />
                                No GPS Fix
                              </Badge>
                            )}
                          </div>
                        </CardHeader>
                        <CardContent className="space-y-4">
                          {gpsLocation ? (
                            <>
                              <div className="space-y-2">
                                <div className="flex justify-between items-center">
                                  <span className="text-sm font-medium">Latitude:</span>
                                  <span className="text-sm text-gray-600">{gpsLocation.latitude.toFixed(6)}</span>
                                </div>
                                <div className="flex justify-between items-center">
                                  <span className="text-sm font-medium">Longitude:</span>
                                  <span className="text-sm text-gray-600">{gpsLocation.longitude.toFixed(6)}</span>
                                </div>
                                {gpsLocation.satellites !== undefined && (
                                  <div className="flex justify-between items-center">
                                    <span className="text-sm font-medium">Satellites:</span>
                                    <span className="text-sm text-gray-600">{gpsLocation.satellites}</span>
                                  </div>
                                )}
                                {gpsLocation.hdop !== undefined && (
                                  <div className="flex justify-between items-center">
                                    <span className="text-sm font-medium">HDOP:</span>
                                    <span className="text-sm text-gray-600">{gpsLocation.hdop.toFixed(2)}</span>
                                  </div>
                                )}
                              </div>
                              
                              {/* Google Maps Embed */}
                              <div className="w-full h-64 rounded-lg overflow-hidden border">
                                <iframe
                                  width="100%"
                                  height="100%"
                                  style={{ border: 0 }}
                                  loading="lazy"
                                  allowFullScreen
                                  referrerPolicy="no-referrer-when-downgrade"
                                  src={`https://maps.google.com/maps?q=${gpsLocation.latitude},${gpsLocation.longitude}&z=15&output=embed`}
                                ></iframe>
                              </div>
                              
                              {/* Open in Google Maps Button */}
                              <Button
                                variant="outline"
                                className="w-full"
                                onClick={() => {
                                  const url = `https://www.google.com/maps?q=${gpsLocation.latitude},${gpsLocation.longitude}`;
                                  window.open(url, '_blank');
                                }}
                              >
                                <ExternalLink className="h-4 w-4 mr-2" />
                                Open in Google Maps
                              </Button>
                            </>
                          ) : (
                            <div className="text-center py-8 space-y-2">
                              <AlertTriangle className="h-12 w-12 mx-auto text-yellow-500" />
                              <p className="text-sm text-gray-600">
                                {gpsStatus.hasFix 
                                  ? "GPS fix acquired, waiting for location data..." 
                                  : "GPS signal not available. The system will continue to work without GPS."}
                              </p>
                              {!gpsStatus.hasFix && gpsStatus.satellites > 0 && (
                                <p className="text-xs text-gray-500">
                                  Detected {gpsStatus.satellites} satellite(s) - acquiring fix...
                                </p>
                              )}
                            </div>
                          )}
                        </CardContent>
                      </Card>
                    </div>
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