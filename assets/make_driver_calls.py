"""Generate prototype driver calls with the offline Windows David voice.

Writes 11025 Hz unsigned 8-bit mono PCM to src/hyah.pcm and src/getalong.pcm.
These are generated assets; normal builds do not need Windows speech synthesis.
Clips are normalized to a peak amplitude of 50 around the silence level of 128.

Usage: python assets/make_driver_calls.py
"""

import os
from pathlib import Path
import subprocess
import tempfile
import wave

ROOT = Path(__file__).resolve().parent.parent
CALLS = (("hyah", "Hyah!"), ("getalong", "Get along!"))


def main():
    with tempfile.TemporaryDirectory(prefix="oregon-voices-") as directory:
        env = os.environ.copy()
        env["OREGON_VOICE_OUTPUT"] = directory
        subprocess.run(
            ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", r"""
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Speech
$voice = New-Object System.Speech.Synthesis.SpeechSynthesizer
try {
    $voice.SelectVoice('Microsoft David Desktop')
    $voice.Rate = 0
    $format = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(
        11025,
        [System.Speech.AudioFormat.AudioBitsPerSample]::Eight,
        [System.Speech.AudioFormat.AudioChannel]::Mono)
    foreach ($call in @(@('hyah', 'Hyah!'), @('getalong', 'Get along!'))) {
        $path = Join-Path $env:OREGON_VOICE_OUTPUT ($call[0] + '.wav')
        $voice.SetOutputToWaveFile($path, $format)
        $voice.Speak($call[1])
        $voice.SetOutputToNull()
    }
} finally {
    $voice.Dispose()
}
"""], env=env, check=True, timeout=30)
        for name, phrase in CALLS:
            with wave.open(str(Path(directory) / (name + ".wav")), "rb") as wav:
                if (wav.getframerate(), wav.getsampwidth(), wav.getnchannels()) != (11025, 1, 1):
                    raise ValueError("unexpected speech output format")
                samples = wav.readframes(wav.getnframes())
            active = [i for i, sample in enumerate(samples) if abs(sample - 128) > 4]
            if not active:
                raise ValueError("speech synthesis produced silence: " + phrase)
            samples = samples[max(0, active[0] - 220):active[-1] + 221]
            peak = max(abs(sample - 128) for sample in samples)
            samples = bytes(128 + (sample - 128) * 50 // peak for sample in samples)
            path = ROOT / "src" / (name + ".pcm")
            path.write_bytes(samples)
            print(f"{phrase}: {len(samples)} bytes, {len(samples) / 11025:.2f}s -> {path}")


if __name__ == "__main__":
    main()
