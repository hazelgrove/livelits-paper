Paper Title: Filling Typed Holes with Live GUIs

Authors: 

Cyrus Omar (University of Michigan)
David Moon (University of Michigan)
Nick Collins (University of Chicago)
Andrew Blinn (University of Michigan)
Ian Voysey (University of Michigan)
Ravi Chugh (University of Chicago)

# Summary 

There are two independent components to this artifact, corresponding to the two
root directories in this archive.

/stllc 

  This is an Agda mechanization of the Simply Typed Livelit Calculus,
  including both:
   
   1. Agda source code
   2. A Docker image which includes an Agda installation that can be used to
      check the proof without locally installing Agda.

  Instructions for local installation and for running the Docker image are
  included in the README file in /stllc.

  This mechanization was described in Sec. 4.2.3 of the paper and formalizes the
  syntax, semantics, and metatheory described in Sec. 4.0-4.2 of the paper and
  in Figure 4 and Figure 5. 
  
  The README file in /stllc includes a mapping from the definitions, figures,
  and theorems in the paper to the corresponding Agda definitions. 
  
  We make liberal use of Unicode to try to make the syntax of terms and
  judgements match the syntax used in the paper in a recognizable fashion, up to
  the limitations of Agda's syntax extension mechanism.

  This mechanization is derived from the mechanization of Hazelnut Live, which
  accompanied a prior paper by some of the authors.

  Note that Sec. 4.3 of the paper, which discusses closure collection, is not
  mechanized. This is because its correctness follows from the Resumption
  theorem of Hazelnut Live, for which there was only a paper proof in the prior
  work. We did not claim that this material was mechanized in the paper.

/hazel-livelits

  This is an implementation of livelits within the Hazel programming
  environment. Hazel is a single page client-side web application. 
  
  /hazel-livelits/www/index.html

    This is a pre-built implementation of Hazel, which can be opened in a modern
    web browser. 

    We have tested it using Version 88.0.4324.182 of Chromium.
       
    We have lightly tested it in Firefox as well, but it may not work perfectly
    in that browser (e.g. due to its more restrictive stack depth limit or other
    differences.)

  /hazel-livelits/hazel
    
    The source code, which is written primarily in ReasonML, an alternative
    syntax for OCaml, and compiled to Javascript for execution in the browser by
    the js_of_ocaml compiler.

    The README.md file includes installation instructions for all of the
    software necessary to build Hazel yourself.

    The pre-built implementation was built using `make release`.

    The implementation of livelits touches virtually every aspect of Hazel, so
    it is difficult to point specifically to the livelits-related code
    additions. The central new construct is the ApLivelit constructor of the
    UHExp.t datatype, which is a livelit invocation.

  ## Reproduction Instructions

  The implementation can be used to reproduce the following figures:

  1. Figure 1(b), with only incidental variation
    
    This example is called "color" and can be loaded from the drop-down menu at
    the bottom of the user interface.

    Observe how interacting with the slider defining the baseline variable
    causes live updates throughout the environment. In particular, notice how
    sliding the $slider traces a semicircle in the RGB color space due to the
    expressions entered into the splices of the $color livelit. 

    Focus management is still a bit wonky, so you need to click on a slider to
    give it focus before you can drag. Some livelit interactions may cause the
    editor cursor's location to jump. To put your cursor in a splice, you have
    to click on the actual code (rather than adjacent whitespace).
    
    Alternatively, selecting a color constant can be done by clicking a color in
    the $color livelit's GUI. This causes the R, G, and B splices to be replaced
    by integer literals.

    When the splice values are indeterminate, e.g. due to a hole in the splice,
    the $color livelit cannot display a color, so it goes into its own
    indeterminate feedback mode. An expansion is still being generated -- it is only the
    live preview that is impossible to generate in this situation.

    Hazel does not support type abbreviations currently, so we use the 4-tuple
    type Int, Int, Int, Int to encode a color value. You can see the type of the
    $color livelit expression in the top right part of the user interface (the
    cursor inspector) when your cursor is on its name.

    Notice also that when the cursor is in a splice, the cursor inspector
    displays the type of the splice so that the user does not need to know how
    the underlying expansion works in detail.

    Observe that $percent is defined as a livelit abbreviation -- it is defined 
    by partially applying the min and max bound parameters that $slider
    otherwise needs.

    This example can also be constructed from scratch. Things to know:

    - Every editor state in Hazel is semantically meaningful. 
      Consequently, evaluation can occur on each edit. 
      
      (Yup, evaluation is in the GUI thread, so this can make the editor
      unboundedly slow... we make no performance related claims.)

    - Holes are displayed as empty spaces with gray numbers, which uniquely
      identify the hole. You can type to fill the hole.

    - Hazel is a structure editor, so holes, punctuation, and whitespace 
      characters are inserted automatically, e.g. when you type "let ".
      Arithmetic folllows the usual order of operations (which you can
      see when your cursor is on the operator). 
      
      Selection is NOT implemented in Hazel at this time. The browser's 
      selection mechanism can be used to copy code out but the edit 
      actions are not selection-aware.
      
      These basic editor mechanics are not part of the claimed contributions of
      this paper.

    - The left sidebar displays a summary of all of the actions / language features
      in Hazel and how they can be invoked. It is hidden by default but can be
      toggled by clicking the tan bar with the "?" at the bottom left.
    
    - You can move around using arrow keys or by clicking. You can also 
      use Tab and Shift+Tab to quickly move to the next or previous hole.

    - Livelit abbreviations, e.g. "let $percent = ..." in this example, 
      are constructed by typing "abbrev " rather than "let " due to 
      limitations in the underlying editor (which are being fixed as 
      part of a separate research project.)

    - The livelit must influence the result of the program in order for
      closures to be collected, which in turn is necessary for the live 
      preview aspect of the $color livelit to operate correctly. 
      
      The easiest way to ensure this is to leave a hole at the end of the 
      program, which implicitly is influenced by every variable in scope.
      You can also fill this hole with default_color if you want to see 
      the underlying color value computed by the color expression that 
      the livelit is generating.

      If no closures are collected, the livelit still operates correctly,
      but it does not offer a live preview of the generated color expression's 
      value.

    - Expansions can be viewed by toggling "show unevaluated expansion" 
      under "Options" in the right sidebar.

    - Also clicking "show function bodies" will reveal the hygiene mechanism in 
      use as described in the paper. We say more about hygiene below.

    - Undo (Ctrl/Cmd+Z) does not currently work. (It can be re-enabled when  
      you build from scratch but it may sometimes crash the application
      and does not handle livelit actions correctly.)

  2. Figure 2, with only incidental variation

    This example is called "grade_cutoffs" and can be loaded from the drop-down menu at
    the bottom of the user interface.

    Observe that the data frame's cells display the computed value of the
    cell. When the cell is selected, the expression that computes that value is
    shown in the formula bar at the top. This demonstrates live evaluation.

    Row and column are also cells, but of type String rather than Float. Observe
    this distinction in the cursor inspector.

    Observe that the $fslider livelit, which is a floating point slider, appears
    directly within the splice of the first cell. As it is manipulated, the cell
    value is updated live.

    Note again that because we do not have type abbreviations, the "Dataframe" 
    type from the paper is structurally written [String], [String, [Float]], 
    i.e. a list of String column headings paired with a list of pairs of 
    String row keys and Float list row values.

    You can add new rows and columns by clicking on the + buttons. This 
    demonstrates that the number of splices in a livelit can change with 
    interaction. (We have not implemented row/col deletion, but the system
    does support removing splices as well.)

    The $grade_cutoffs example is parameterized by the averages computed 
    by the call to compute_weighted_averages (for simplicity, this is included 
    in the prelude and not shown here.)

    It can evaluate this value and use the resulting data to plot the dots on
    the number line. You can hover over each dot to see the student's name.

    It also computes the number of students in each grade bin live as you 
    slide the paddles around (which can be pushed by other paddles so the 
    grades remain ordered correctly.)

    Another interesting interaction to observe: as you slide the $fslider for 
    Alice's A1 grade, you can see how it moves her dot along the number line 
    in the $grade_cutoffs GUI, demonstrating visually how big of an impact 
    doing better or worse on one question of one assignment can have. She 
    could even have flipped from an A to a B if that question had gone
    differently.
    
    The computed result at the bottom assigns a letter grade to each student by
    calling assign_grades (again included in the prelude for simplicity here.)

    Known issues:
    - Focus management is again not great in the $data_frame livelit, e.g. 
      when you click a cell it does not focus the corresponding splice. We are 
      revamping focus management to better support these kinds of use cases.

    - The full expansion is large enough that our pretty printer times out if 
      you attempt to display it.

    - The indeterminate feedback modes for both livelits here could be improved
      e.g. $data_frame could show the formula inline when a value is
      unavailable, and $grade_cutoffs could even attempt to "skip" grades that 
      are indetermine while still plotting those that are determinate.
      Currently, if any grades are indeterminate then the live view is disabled.
      In both cases, this is only a visual difference -- the expansion is still 
      being generated.

  3. Figure 3, with more substantial variation consistent with our characterization 
     of this figure in the paper as using unimplemented syntactic sugar and omitting
     certain incidental details.

  How?

  

  A brief introduction to Hazel

2. A snapshot of the Hazel implementation of livelits, which can be used to reproduce (with incidental
   variation) Fig. 1, Fig. 2, and a simplified variation on Fig. 3 (consistent with our characterization 
   of Fig. 3 in the pape r text as using unimplemented syntactic sugar and omitting certain incidental details).


A mechanization of the Simply Typed Livelit Calculus as described in Sec. 4.2.3 of the paper. 