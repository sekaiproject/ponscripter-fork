;;; ponscripter-mode.el --- Ponscripter code highlighting for Emacs

;; Copyright (C) 2008 Haeleth

;; Author: Peter Jolly
;; Maintainer: Peter Jolly <haeleth@haeleth.net>
;; Keywords: languages, Ponscripter, ONScripter, NScripter

;; Bugs:
;;
;;   ~~i~ means literal "~i" followed by an open tag block, but is
;;   here parsed as unmatched ~ followed by tag block ~i~.
;;
;;   ~i~~~ means tag block ~i~ followed by literal "~", but is here
;;   parsed as tag block ~i~ followed by two unmatched ~s.

(defvar ponscripter-mode-hook nil)
(defvar ponscripter-mode-map
  (let ((ponscripter-mode-map (make-sparse-keymap)))
    ponscripter-mode-map)
  "Keymap for Ponscripter major mode")

(defvar ponscripter-font-lock-keywords
  (list
   '("\\^.*?\\^"          . font-lock-string-face)        ; ^strings^
   '("~[^~]+~"            . font-lock-constant-face)      ; ~tags~
   '("~~"                 . font-lock-keyword-face)       ; literal ~
   '("~"                  . font-lock-warning-face)       ; unpaired ~
   '("[$%?]\\w+"          . font-lock-variable-name-face) ; variables
   '("[*]\\w+"            . font-lock-constant-face)      ; labels
   '("#[@/\\_^`!#]"       . font-lock-keyword-face)       ; #@, #\, etc
   '("![swd][0-9]+"       . font-lock-constant-face)      ; !s, !w, !d
   '("!sd"                . font-lock-constant-face)      ; !sd
   '("#[0-9a-f]\\{6\\}"   . font-lock-constant-face)      ; #nnnnnn
   '("^[ \t]*\\([`^]\\)"  1 font-lock-builtin-face)       ; ^text
   '("[\\@_]"             . font-lock-builtin-face)       ; \, @, _
   '("/$"                 . font-lock-builtin-face)       ; / at eol
   ))

(defvar ponscripter-syntactic-keywords
  (list '("^\\(?:[^^\"]\\|\\^.*?\\^\\|\".*?\"\\)*?\\(;\\)"
          1 "<"))) ; semicolon only begins comment outside text

(defvar ponscripter-mode-syntax-table
  (let ((table (make-syntax-table)))
    (modify-syntax-entry ?_  "w" table)
    (modify-syntax-entry ?\n ">" table)
    table))

(defun ponscripter-mode ()
  "Major mode for editing Ponscripter files.
Also has some (imperfect) support for ONScripter-En and NScripter
proper."
  (interactive)
  (kill-all-local-variables)
  (set-syntax-table ponscripter-mode-syntax-table)
  (use-local-map ponscripter-mode-map)
  (set (make-local-variable 'font-lock-defaults)
       '(ponscripter-font-lock-keywords nil t nil nil
	    (font-lock-syntactic-keywords . ponscripter-syntactic-keywords)))
  (setq major-mode 'ponscripter-mode)
  (setq mode-name "Ponscripter")
  (run-hooks 'ponscripter-mode-hook))

(provide 'ponscripter-mode)

(defun ispell-ponscripter-text ()
  "Check Ponscripter text in the current buffer for spelling errors.
Checks all strings and single-byte display text, with the
exception of single-word strings containing numbers or
backslashes (as these are probably filenames)."
  (interactive)
  (save-excursion
    (goto-char (point-min))
    (let ((working t)
	  (ispell-silently-savep t))
      (while 
	  (and working
	       (posix-search-forward "\\([\"^]\\).+?\\1\\|^[ \t]*\\^.*$\\|;.*$"
				     nil t))
	(unless (or
	    (string-match "^\\([\"^]\\)[^ ]*[0-9\\][^ ]*\\1$" (match-string 0))
	    (string-match "^;;?[a-z0-9_]+ " (match-string 0)))
	  (setq working (ispell-region (match-beginning 0) (match-end 0))))))))
