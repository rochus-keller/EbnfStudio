![icon](http://software.rochus-keller.info/ebnfstudio_icon_128.png)
## Welcome to EbnfStudio 

EbnfStudio is a development environment for EBNF files which supports syntax highlighting, inline warnings, symbol navigation and cross-referencing. The grammar is automatically analyzed for syntax errors, missing non-terminals and left recursion while editing. The grammar can also be checked for LL(1) ambiguities and the effectiveness of conflict resolvers. EbnfStudio allows to keep grammars implementation-free; this also applies to the conflict resolvers which are terminals of the form \LL:k\ with k > 1. The current implementation supports the generation of input files to the Coco/R, LLnextgen and ANTLR parser generators, but currently conflict resolvers and the automatic generation of syntax trees are only supported for Coco/R.

EbnfStudio is work in progress. It is currently used as a replacement for VerilogEbnf and to add parts of SystemVerilog to the existing LL(1) Verilog 05 syntax.

![alt text](http://software.rochus-keller.info/ebnfstudio_screenshot_1.png "EbnfStudio Screenshot")

### Build Steps
Follow these steps if you intend to build EbnfStudio:

1. Create a directory; let's call it BUILD_DIR
1. Download the EbnfStudio source code from https://github.com/rochus-keller/EbnfStudio/archive/master.zip and unpack it to the BUILD_DIR; rename the subdirectory to "EbnfStudio".
1. Download the GuiTools source code from https://github.com/rochus-keller/GuiTools/archive/master.zip and unpack it to the BUILD_DIR; rename it to "GuiTools". 
1. Goto the BUILD_DIR/EbnfStudio subdirectory and execute `QTDIR/bin/qmake EbnfStudio.pro` (see the Qt documentation concerning QTDIR).
1. Run make; after a couple of seconds you will find the executable in the tmp subdirectory.

Alternatively you can open EbnfStudio.pro using QtCreator and build it there.

## Support
If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/EbnfStudio/issues or send an email to the author.



