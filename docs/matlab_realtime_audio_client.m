%% ESP32 Realtime Audio TCP Client
% Connects to the ESP32 audio stream (s16le PCM frames) and shows
% waveform + spectrogram in near real time.
%
% Protocol per frame:
%   4 bytes  magic: 'AUD0'
%   4 bytes  uint32 frameIndex (little-endian)
%   2 bytes  uint16 sampleRate
%   2 bytes  uint16 samplesPerFrame
%   2 bytes  uint16 channels
%   2 bytes  uint16 format (1 = signed int16 PCM little-endian)
%   N bytes  payload: int16 samples (mono)

host = "192.168.1.11";   % Set this to your ESP32 IP
port = 3333;

windowSeconds = 6;         % Display window length
specNfft = 512;
specOverlap = 384;
specDynamicRangeDb = 80;    % Color range below the current peak (dB)

c = tcpclient(host, port, "Timeout", 8);
fprintf("Connected to %s:%d\n", host, port);

maxSamples = 16000 * windowSeconds;
audioBuffer = zeros(1, maxSamples, "double");
writePos = 1;

f = figure("Name", "ESP32 TCP Audio Stream", "Color", "w");
tiledlayout(f, 2, 1, "Padding", "compact", "TileSpacing", "compact");
ax1 = nexttile;
ax2 = nexttile;

while isvalid(f)
    [headerBytes, ok] = readExact(c, 16);
    if ~ok
        warning("TCP read timed out while waiting for header.");
        break;
    end

    magic = char(headerBytes(1:4));
    if ~strcmp(magic, "AUD0")
        warning("Frame sync lost: bad magic '%s'.", magic);
        break;
    end

    frameIndex = typecast(uint8(headerBytes(5:8)), "uint32");
    sampleRate = double(typecast(uint8(headerBytes(9:10)), "uint16"));
    samplesPerFrame = double(typecast(uint8(headerBytes(11:12)), "uint16"));
    channels = double(typecast(uint8(headerBytes(13:14)), "uint16"));
    formatCode = double(typecast(uint8(headerBytes(15:16)), "uint16"));

    if channels ~= 1 || formatCode ~= 1
        warning("Unsupported format: channels=%d formatCode=%d", channels, formatCode);
        break;
    end

    payloadLen = samplesPerFrame * 2;
    [payloadBytes, ok] = readExact(c, payloadLen);
    if ~ok
        warning("TCP read timed out while waiting for payload.");
        break;
    end

    x = double(typecast(uint8(payloadBytes), "int16")) / 32768.0;

    n = numel(x);
    idx = writePos:(writePos + n - 1);
    idxWrap = mod(idx - 1, maxSamples) + 1;
    audioBuffer(idxWrap) = x;
    writePos = mod(writePos + n - 1, maxSamples) + 1;

    ring = [audioBuffer(writePos:end), audioBuffer(1:writePos-1)];
    t = (0:numel(ring)-1) / sampleRate;

    plot(ax1, t, ring, "b-");
    title(ax1, sprintf("Waveform (frame %u)", frameIndex));
    xlabel(ax1, "Time (s)");
    ylabel(ax1, "Amplitude (FS)");
    ylim(ax1, [-1 1]);
    grid(ax1, "on");

    [~, fHz, tSpec, p] = spectrogram(ring, hann(specNfft), specOverlap, specNfft, sampleRate, "power");
    pDb = 10 * log10(p + eps);

    imagesc(ax2, tSpec, fHz / 1000, pDb);
    axis(ax2, "xy");

    % Use robust peak tracking so short transients do not collapse the color range.
    peakDb = prctile(pDb(:), 99.5);
    caxis(ax2, [peakDb - specDynamicRangeDb, peakDb]);

    title(ax2, "Spectrogram");
    xlabel(ax2, "Time (s)");
    ylabel(ax2, "Frequency (kHz)");
    colormap(ax2, turbo);
    cb = colorbar(ax2);
    cb.Label.String = "Power/Frequency (dB/Hz)";

    drawnow limitrate;
end

clear c;

function [out, ok] = readExact(client, nBytes)
    out = zeros(1, nBytes, "uint8");
    count = 0;
    deadline = tic;

    while count < nBytes
        avail = client.NumBytesAvailable;
        if avail > 0
            take = min(avail, nBytes - count);
            chunk = read(client, take, "uint8");
            out(count + (1:numel(chunk))) = chunk;
            count = count + numel(chunk);
            continue;
        end

        if toc(deadline) > client.Timeout
            ok = false;
            return;
        end
        pause(0.001);
    end

    ok = true;
end
