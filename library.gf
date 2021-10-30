---
glyph: metacode
text: |
   ; Here begins the prelude to the Glyph standard library.
   ; Right now, we have only three atomic glyphs:
   ;  1. The ability to output raw LLVM IR into the metacontext.
   ;  2. The ability to compile the metacontext and run a specific function.
   ;  3. The ability to include another Glyph file.
   ; We don't even have the ability to write comments yet!
   ; Except, of course, as comments within the LLVM IR.
   ; So our first line of business is going to be implementing a comment glyph.

   ; ===== COMMENT GLYPH =====
   ; In order to define a glyph, we must execute code in the metacontext.
   ; We will use the built-in function:
   declare void @atom_define_glyph(i8* nocapture, void()* nocapture) nounwind
   ; The two arguments of this function are:
   ;   1. The name of the glyph as a null-terminated C string.
   ;   2. A function pointer to the INTERPRETATION FUNCTION. This is the
   ;      function that can a) generate LLVM IR when the glyph is used, and
   ;                        b) visualize the glyph when a piece of code
   ;                           including the glyph is shown in an editor.
   ; Let's do these one at a time.

   ; First, let's define the name:
   @std_comment_name = private unnamed_addr constant [8 x i8] c"comment\00"

   ; In order to define the interpretation function, we will need certain
   ; built-in functions to perform the fundamental operations.
   ; Our interpretation function for the comment glyph will not generate any
   ; code, because comments do nothing, but it will need to render the comment
   ; text. In order to do that rendering, we need a few functions...

   ; We need the built-in function that sets the color of things we draw using
   ; separate red, green, and blue channels:
   declare void @atom_set_color(i8, i8, i8) nounwind

   ; We need the function that retrieves the text for the current glyph:
   declare i8* @atom_fetch_text() nounwind

   ; And we need the function that lets us render text:
   declare void @atom_render_text(i8* nocapture) nounwind

   ; Now we can render comments in another color.
   define void @std_comment_interp() {
     ; pick a nice light blue for the comment text
     call void @atom_set_color(i8 112, i8 176, i8 176)
     ; fetch the comment string
     %comment = call i8* @atom_fetch_text()
     ; and draw the comment string
     call void @atom_render_text(i8* %comment)
     ret void
   }

   ; Finally, let's pull these pieces together by writing the code that we'll
   ; execute to define our new piece of syntax:
   define void @std_comment_define() {
     ; Grab a pointer to the name first
     %name = getelementptr [8 x i8], [8 x i8]* @std_comment_name, i64 0, i64 0
     call void @atom_define_glyph(i8* %name, void()* @std_comment_interp)
     ret void
   }
   ; And the final step: run it!
...
---
glyph: metaexec
text: std_comment_define
...
---
glyph: comment
text: hello world
...
