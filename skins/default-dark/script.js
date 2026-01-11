// SODA Player - Default Dark Theme Script

(function() {
    'use strict';

    // DOM elements
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

    // State
    let isPlaying = false;
    let currentPosition = 0;
    let totalDuration = 0;

    // Format time in mm:ss
    function formatTime(ms) {
        const totalSeconds = Math.floor(ms / 1000);
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;
        return `${minutes}:${seconds.toString().padStart(2, '0')}`;
    }

    // Update UI based on playback state
    function updatePlayState(playing) {
        isPlaying = playing;
        elements.btnPlay.textContent = playing ? '⏸' : '▶';
    }

    // Update track info
    function updateTrackInfo(track) {
        if (track) {
            elements.trackTitle.textContent = track.title || 'Unknown Title';
            elements.trackArtist.textContent = track.artist || 'Unknown Artist';

            if (track.coverUrl) {
                elements.coverArt.innerHTML = `<img src="${track.coverUrl}" alt="Cover">`;
            } else {
                elements.coverArt.innerHTML = '<div class="placeholder">♪</div>';
            }

            totalDuration = track.duration || 0;
            elements.timeTotal.textContent = formatTime(totalDuration);
        } else {
            elements.trackTitle.textContent = 'No track playing';
            elements.trackArtist.textContent = '-';
            elements.coverArt.innerHTML = '<div class="placeholder">♪</div>';
            elements.timeTotal.textContent = '0:00';
            totalDuration = 0;
        }
    }

    // Update progress
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

    // Render track list
    function renderTrackList(tracks) {
        if (!tracks || tracks.length === 0) {
            elements.trackList.innerHTML = `
                <div class="empty-state">
                    <p>No tracks in your library</p>
                    <p class="hint">Add music folders in Settings to get started</p>
                </div>
            `;
            return;
        }

        elements.trackList.innerHTML = tracks.map(track => `
            <div class="track-item" data-id="${track.id}">
                <div class="track-cover">♪</div>
                <div class="track-info">
                    <div class="track-title">${escapeHtml(track.title)}</div>
                    <div class="track-artist">${escapeHtml(track.artist || 'Unknown Artist')}</div>
                </div>
                <div class="track-duration">${formatTime(track.duration || 0)}</div>
            </div>
        `).join('');

        // Add click handlers
        elements.trackList.querySelectorAll('.track-item').forEach(item => {
            item.addEventListener('click', () => {
                const trackId = item.dataset.id;
                soda.queueAdd(trackId, (success) => {
                    if (success) {
                        soda.play();
                    }
                });
            });
        });
    }

    // Render queue
    function renderQueue(queue) {
        if (!queue || !queue.tracks || queue.tracks.length === 0) {
            elements.queueList.innerHTML = `
                <div class="empty-state">
                    <p>Queue is empty</p>
                </div>
            `;
            return;
        }

        elements.queueList.innerHTML = queue.tracks.map((track, index) => `
            <div class="track-item ${index === queue.currentIndex ? 'playing' : ''}" data-index="${index}">
                <div class="track-info">
                    <div class="track-title">${escapeHtml(track.title)}</div>
                    <div class="track-artist">${escapeHtml(track.artist || '')}</div>
                </div>
            </div>
        `).join('');

        // Add click handlers
        elements.queueList.querySelectorAll('.track-item').forEach(item => {
            item.addEventListener('click', () => {
                const index = parseInt(item.dataset.index);
                soda.queueJumpTo(index);
            });
        });
    }

    // Escape HTML
    function escapeHtml(str) {
        if (!str) return '';
        return str
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }

    // Search
    function performSearch() {
        const query = elements.searchInput.value.trim();
        if (!query) {
            loadLibrary();
            return;
        }

        soda.search(query, (success, results) => {
            if (success && results) {
                renderTrackList(results);
            }
        });
    }

    // Load library
    function loadLibrary() {
        soda.getTracks((success, tracks) => {
            if (success) {
                renderTrackList(tracks);
            }
        });
    }

    // Load queue
    function loadQueue() {
        soda.queueGet((success, queue) => {
            if (success) {
                renderQueue(queue);
            }
        });
    }

    // Event handlers
    elements.searchBtn.addEventListener('click', performSearch);
    elements.searchInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') performSearch();
    });

    elements.progressSlider.addEventListener('input', (e) => {
        const percent = e.target.value;
        const position = (percent / 100) * totalDuration;
        soda.seek(position);
    });

    elements.volumeSlider.addEventListener('input', (e) => {
        const volume = e.target.value / 100;
        soda.setVolume(volume);
    });

    // Navigation
    document.querySelectorAll('.nav-item').forEach(item => {
        item.addEventListener('click', () => {
            document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
            item.classList.add('active');

            const view = item.dataset.view;
            // Handle view switching here
        });
    });

    // SODA event handler
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
                updateProgress(0, 0);
                break;

            case 'playbackProgress':
                if (event.data) {
                    updateProgress(event.data.position, event.data.duration);
                }
                break;

            case 'trackChanged':
                if (event.data) updateTrackInfo(event.data);
                break;

            case 'queueChanged':
                loadQueue();
                break;

            case 'volumeChanged':
                if (event.data && event.data.volume !== undefined) {
                    elements.volumeSlider.value = event.data.volume * 100;
                }
                break;

            case 'error':
                console.error('SODA Error:', event.data?.message);
                break;
        }
    };

    // Initial load
    soda.getState((success, state) => {
        if (success && state) {
            updatePlayState(state.state === 'playing');
        }
    });

    soda.getCurrentTrack((success, track) => {
        if (success && track) {
            updateTrackInfo(track);
        }
    });

    soda.getVolume((success, data) => {
        if (success && data) {
            elements.volumeSlider.value = data.volume * 100;
        }
    });

    loadLibrary();
    loadQueue();

    console.log('SODA Dark theme initialized');
})();
