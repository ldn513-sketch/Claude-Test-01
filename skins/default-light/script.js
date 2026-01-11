// SODA Player - Light theme uses the same script as dark theme
// See default-dark/script.js for the full implementation

(function() {
    'use strict';

    const elements = {
        trackTitle: document.getElementById('track-title'),
        trackArtist: document.getElementById('track-artist'),
        coverArt: document.getElementById('cover-art'),
        btnPlay: document.getElementById('btn-play'),
        timeCurrent: document.getElementById('time-current'),
        timeTotal: document.getElementById('time-total'),
        progressFill: document.getElementById('progress-fill'),
        progressSlider: document.getElementById('progress-slider'),
        volumeSlider: document.getElementById('volume-slider'),
        trackList: document.getElementById('track-list'),
        queueList: document.getElementById('queue-list'),
        searchInput: document.getElementById('search-input'),
        searchBtn: document.getElementById('search-btn'),
        btnShuffle: document.getElementById('btn-shuffle'),
        btnRepeat: document.getElementById('btn-repeat')
    };

    let isPlaying = false;
    let currentPosition = 0;
    let totalDuration = 0;

    function formatTime(ms) {
        const totalSeconds = Math.floor(ms / 1000);
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;
        return `${minutes}:${seconds.toString().padStart(2, '0')}`;
    }

    function updatePlayState(playing) {
        isPlaying = playing;
        elements.btnPlay.textContent = playing ? '⏸' : '▶';
    }

    function updateTrackInfo(track) {
        if (track) {
            elements.trackTitle.textContent = track.title || 'Unknown Title';
            elements.trackArtist.textContent = track.artist || 'Unknown Artist';
            totalDuration = track.duration || 0;
            elements.timeTotal.textContent = formatTime(totalDuration);
        } else {
            elements.trackTitle.textContent = 'No track playing';
            elements.trackArtist.textContent = '-';
            elements.timeTotal.textContent = '0:00';
            totalDuration = 0;
        }
    }

    function updateProgress(position, duration) {
        currentPosition = position;
        if (duration) totalDuration = duration;
        elements.timeCurrent.textContent = formatTime(position);
        if (totalDuration > 0) {
            const percent = (position / totalDuration) * 100;
            elements.progressFill.style.width = `${percent}%`;
            elements.progressSlider.value = percent;
        }
    }

    function escapeHtml(str) {
        if (!str) return '';
        return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }

    function renderTrackList(tracks) {
        if (!tracks || tracks.length === 0) {
            elements.trackList.innerHTML = '<div class="empty-state"><p>No tracks</p></div>';
            return;
        }
        elements.trackList.innerHTML = tracks.map(track => `
            <div class="track-item" data-id="${track.id}">
                <div class="track-cover">♪</div>
                <div class="track-info">
                    <div class="track-title">${escapeHtml(track.title)}</div>
                    <div class="track-artist">${escapeHtml(track.artist || '')}</div>
                </div>
                <div class="track-duration">${formatTime(track.duration || 0)}</div>
            </div>
        `).join('');
    }

    elements.searchBtn.addEventListener('click', () => {
        const query = elements.searchInput.value.trim();
        if (query) {
            soda.search(query, (success, results) => {
                if (success) renderTrackList(results);
            });
        }
    });

    elements.progressSlider.addEventListener('input', (e) => {
        const position = (e.target.value / 100) * totalDuration;
        soda.seek(position);
    });

    elements.volumeSlider.addEventListener('input', (e) => {
        soda.setVolume(e.target.value / 100);
    });

    soda.onEvent = function(event) {
        switch (event.type) {
            case 'playbackStarted':
                updatePlayState(true);
                if (event.data) updateTrackInfo(event.data);
                break;
            case 'playbackPaused':
                updatePlayState(false);
                break;
            case 'playbackStopped':
                updatePlayState(false);
                updateTrackInfo(null);
                break;
            case 'playbackProgress':
                if (event.data) updateProgress(event.data.position, event.data.duration);
                break;
            case 'trackChanged':
                if (event.data) updateTrackInfo(event.data);
                break;
        }
    };

    soda.getTracks((success, tracks) => { if (success) renderTrackList(tracks); });
    console.log('SODA Light theme initialized');
})();
