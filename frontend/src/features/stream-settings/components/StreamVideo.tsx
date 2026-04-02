import { useEffect, useRef } from 'react';

const StreamVideo = ({ stream }: { stream: MediaStream | null }) => {
    const videoRef = useRef<HTMLVideoElement>(null);

    useEffect(() => {
        if (videoRef.current && stream) {
            videoRef.current.srcObject = stream;
        }
    }, [stream]);

    return (
        <video ref={videoRef} id="out" muted autoPlay playsInline style={{width:'100%'}} />
    );
};

export default StreamVideo;