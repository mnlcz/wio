(define-module (wio docs))

;;; Documentation for C-registered primitives (wio-*)

(set-procedure-property! (@@ (guile-user) wio-ping) 'documentation
  "Return #t. Used to sanity-check the REPL connection.")

(set-procedure-property! (@@ (guile-user) wio-echo) 'documentation
  "Print STRING to wio's log via C fprintf, return STRING.")

(set-procedure-property! (@@ (guile-user) wio-menu-dispatch) 'documentation
  "(internal hook) Called by C when a main-menu slot is clicked.
Return a symbol naming the resulting input state, or #f.")

(set-procedure-property! (@@ (guile-user) wio-views) 'documentation
  "Return a list of wio-view objects, most-recently-focused first.")

(set-procedure-property! (@@ (guile-user) wio-view-title) 'documentation
  "Return VIEW's title string, or #f if VIEW no longer exists.")

(set-procedure-property! (@@ (guile-user) wio-hide-view) 'documentation
  "Hide VIEW. Return #t, or #f if VIEW no longer exists.")

(set-procedure-property! (@@ (guile-user) wio-restore-view) 'documentation
  "Restore VIEW from hidden. Return #t, or #f if VIEW no longer exists.")
