(define-module (wio docs))

;;; Documentation for C-registered primitives (wio-*)

(set-procedure-property! wio-ping 'documentation
  "Return #t. Used to sanity-check the REPL connection.")

(set-procedure-property! wio-echo 'documentation
  "Print STRING to wio's log via C fprintf, return STRING.")

(set-procedure-property! wio-menu-dispatch 'documentation
  "(internal hook) Called by C when a main-menu slot is clicked.
Return a symbol naming the resulting input state, or #f.")

(set-procedure-property! wio-views 'documentation
  "Return a list of wio-view objects, most-recently-focused first.")

(set-procedure-property! wio-view-title 'documentation
  "Return VIEW's title string, or #f if VIEW no longer exists.")

(set-procedure-property! wio-hide-view 'documentation
  "Hide VIEW. Return #t, or #f if VIEW no longer exists.")

(set-procedure-property! wio-restore-view 'documentation
  "Restore VIEW from hidden. Return #t, or #f if VIEW no longer exists.")
