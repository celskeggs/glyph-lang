---
glyph: metacode
text: |
   ; common declarations for accessing the compiler API
   declare void @atom_define_glyph(i8* nocapture, void()* nocapture) nounwind
   declare void @atom_set_color(i8, i8, i8) nounwind
   declare i8* @atom_fetch_text() nounwind
   declare void @atom_render_text(i8* nocapture) nounwind

   ; name for the new glyph to define
   @std_comment_name = private unnamed_addr constant [8 x i8] c"comment\00"

   ; interpretation function (code generation and glyph rendering)
   define void @std_comment_interp() {
     call void @atom_set_color(i8 112, i8 176, i8 176)
     %comment = call i8* @atom_fetch_text()
     call void @atom_render_text(i8* %comment)
     ret void
   }

   ; wrapper function to invoke using the 'metaexec' glyph
   define void @std_comment_define() {
     %name = getelementptr [8 x i8], [8 x i8]* @std_comment_name, i64 0, i64 0
     call void @atom_define_glyph(i8* %name, void()* @std_comment_interp)
     ret void
   }
...
---
glyph: metaexec
text: std_comment_define
...
---
glyph: comment
text: Hello! This is a comment written using the newly-defined glyph type!
...
