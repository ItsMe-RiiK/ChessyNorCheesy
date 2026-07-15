// ==UserScript==
// @name         ChessBot DOM Link
// @namespace    http://tampermonkey.net/
// @version      1.1
// @description  Send board state to local C++ bot
// @match        *://*.chess.com/*
// @grant        GM_xmlhttpRequest
// ==/UserScript==

(function() {
    'use strict';

    let lastFen = "";
    let observer = null;

    function getPieceChar(classes) {
        if (classes.includes('wp')) return 'P';
        if (classes.includes('wn')) return 'N';
        if (classes.includes('wb')) return 'B';
        if (classes.includes('wr')) return 'R';
        if (classes.includes('wq')) return 'Q';
        if (classes.includes('wk')) return 'K';
        if (classes.includes('bp')) return 'p';
        if (classes.includes('bn')) return 'n';
        if (classes.includes('bb')) return 'b';
        if (classes.includes('br')) return 'r';
        if (classes.includes('bq')) return 'q';
        if (classes.includes('bk')) return 'k';
        return null;
    }

    function parseBoardToFEN() {
        const board = [];
        for (let r = 0; r < 8; r++) {
            board.push(Array(8).fill(null));
        }

        const pieces = document.querySelectorAll('.piece');
        pieces.forEach(p => {
            const cls = p.className;
            const pChar = getPieceChar(cls);
            if (!pChar) return;
            
            const sqMatch = cls.match(/square-(\w)(\d)/);
            if (sqMatch) {
                const file = sqMatch[1].charCodeAt(0) - 'a'.charCodeAt(0);
                const rank = parseInt(sqMatch[2], 10) - 1; // 0-indexed rank from bottom
                board[7 - rank][file] = pChar; // 0 is top rank in FEN
            }
        });

        let fen = "";
        for (let r = 0; r < 8; r++) {
            let emptyCount = 0;
            for (let f = 0; f < 8; f++) {
                if (board[r][f] === null) {
                    emptyCount++;
                } else {
                    if (emptyCount > 0) {
                        fen += emptyCount;
                        emptyCount = 0;
                    }
                    fen += board[r][f];
                }
            }
            if (emptyCount > 0) fen += emptyCount;
            if (r < 7) fen += "/";
        }

        return fen;
    }

    function checkAndSend() {
        const boardElement = document.querySelector('wc-chess-board') || document.querySelector('chess-board');
        if (!boardElement) return;

        const currentFen = parseBoardToFEN();
        if (currentFen !== lastFen && currentFen.length > 10) {
            lastFen = currentFen;
            console.log("[ChessBot] Sending FEN to bot: ", currentFen);
            
            GM_xmlhttpRequest({
                method: "POST",
                url: "http://127.0.0.1:8080/update_fen",
                headers: {
                    "Content-Type": "text/plain"
                },
                data: currentFen,
                onload: function(response) {
                    // Success
                },
                onerror: function(error) {
                    console.error("[ChessBot] Failed to send FEN to bot:", error);
                }
            });
        }
    }

    function attachObserver() {
        const board = document.querySelector('wc-chess-board') || document.querySelector('chess-board');
        if (board && !board.dataset.observed) {
            if (observer) observer.disconnect();
            
            observer = new MutationObserver(() => {
                clearTimeout(window.fenTimer);
                // 100ms debounce allows move animations to finish placing elements
                window.fenTimer = setTimeout(checkAndSend, 100);
            });
            
            observer.observe(board, { childList: true, subtree: true, attributes: true, attributeFilter: ['class'] });
            board.dataset.observed = 'true';
            console.log("[ChessBot] DOM Hook attached!");
        }
    }

    setInterval(attachObserver, 2000);
})();
