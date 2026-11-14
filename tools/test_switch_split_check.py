#!/usr/bin/env python3
"""Tests of tools/switch_split_check.py on a small synthetic file, run from the repository root:

    python3 tools/test_switch_split_check.py

The fixture is the shape of widget.c's `XNoticeWidget`: a `switch` of nine labels and a `default:`
whose groups call helpers that write the caller's `state` through a `const` targets struct. The
correct split, in one step and in two, must print OK; each planted defect, a lost label, a doubled
label, a wrong call, a group edited on the way, a transposed argument, a moved `default:`, a kept group
edited, a statement added to the caller, a binding of the wrong member, a changed initialiser and a
reordering, must be refused.

A second fixture has the shape of display-image.c's `XMagickCommand`: a context struct the caller
fills member by member, written variables passed as pointers (`(*v)`, `&(*v)`) and a member added
by the step. The correct step must print OK; a member filled by address used without `(*v)`, a
member bound but not filled, an appended member filled out of place or not at all, an existing
member changed, and a written variable's group edited must be refused.
"""
import os, sys, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import switch_split_check as S  # noqa: E402

PRE = """
typedef struct _Targets
{
  int
    *dismiss;

  size_t
    *state;
} Targets;

"""

OLD = PRE + """
void Caller(Display *display,XWindows *windows,int event_type,XEvent event)
{
  size_t
    state;

  int
    dismiss;

  const Targets
    targets = { &dismiss, &state };

  /*
    Respond.
  */
  switch (event.type)
  {
    case ButtonPress:
    {
      Press(display,windows,event,&targets);
      break;
    }
    case ButtonRelease:
    {
      Release(display,windows,event,targets);
      break;
    }
    case ClientMessage:
    {
      state=Client(display,windows,event,state);
      break;
    }
    case ConfigureNotify:
    {
      state=Configure(windows,event,state);
      break;
    }
    case KeyPress:
    {
      Key(display,windows,event,&targets);
      break;
    }
    case LeaveNotify:
    {
      state=Leave(windows,event,state);
      break;
    }
    case MotionNotify:
    {
      Move(display,windows,event,&targets);
      break;
    }
    default:
      break;
  }
  Finish(state);
}
"""

HELPER_B = """static void RespondB(Display *display,XWindows *windows,
  const XEvent event,const Targets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      Key(display,windows,event,targets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=Leave(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      Move(display,windows,event,targets);
      break;
    }
    default:
      break;
  }
}

"""

HELPER_A = """static void RespondA(Display *display,XWindows *windows,
  const XEvent event,const Targets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ClientMessage:
    {
      (*state)=Client(display,windows,event,(*state));
      break;
    }
    case ConfigureNotify:
    {
      (*state)=Configure(windows,event,(*state));
      break;
    }
    default:
    {
      RespondB(display,windows,event,targets);
      break;
    }
  }
}

"""

CALLER_NEW = """
void Caller(Display *display,XWindows *windows,int event_type,XEvent event)
{
  size_t
    state;

  int
    dismiss;

  const Targets
    targets = { &dismiss, &state };

  /*
    Respond.
  */
  switch (event.type)
  {
    case ButtonPress:
    {
      Press(display,windows,event,&targets);
      break;
    }
    case ButtonRelease:
    {
      Release(display,windows,event,targets);
      break;
    }
    default:
    {
      RespondA(display,windows,event,&targets);
      break;
    }
  }
  Finish(state);
}
"""

NEW = PRE + HELPER_B + HELPER_A + CALLER_NEW.lstrip("\n")

# the one-step intermediate: the key group helper only
CALLER_STEP1 = CALLER_NEW.replace("""    default:
    {
      RespondA(display,windows,event,&targets);
      break;
    }
""", """    case ClientMessage:
    {
      state=Client(display,windows,event,state);
      break;
    }
    case ConfigureNotify:
    {
      state=Configure(windows,event,state);
      break;
    }
    default:
    {
      RespondB(display,windows,event,&targets);
      break;
    }
""")
STEP1 = PRE + HELPER_B + CALLER_STEP1.lstrip("\n")


def run(old, new, helpers):
    problems, summary = S.check(old, new, "Caller", helpers)
    return problems


class SwitchSplitCheck(unittest.TestCase):
    def test_one_step(self):
        self.assertEqual(run(OLD, STEP1, ["RespondB"]), [])

    def test_second_step_moves_the_default_call(self):
        self.assertEqual(run(STEP1, NEW, ["RespondA"]), [])

    def test_two_steps_at_once(self):
        self.assertEqual(run(OLD, NEW, ["RespondA", "RespondB"]), [])

    def refused(self, old, new, helpers, text):
        problems = run(old, new, helpers)
        self.assertTrue(any(text in p for p in problems), (text, problems))

    def planted(self, what, was, now, text):
        """Refuse the correct two-helper split of OLD once `was` is replaced by `now`."""
        self.assertIn(was, NEW, what)
        self.refused(OLD, NEW.replace(was, now, 1), ["RespondA", "RespondB"], text)

    def test_lost_label(self):
        self.planted("ConfigureNotify group removed from the helper",
                     """    case ConfigureNotify:
    {
      (*state)=Configure(windows,event,(*state));
      break;
    }
""", "", "lost")

    def test_doubled_label(self):
        self.planted("ClientMessage also kept in the caller",
                     """    case ButtonRelease:""",
                     """    case ClientMessage:
    {
      break;
    }
    case ButtonRelease:""", "two groups")

    def test_label_added(self):
        self.planted("a label the original did not name",
                     "    case KeyPress:", "    case KeyRelease:\n    case KeyPress:", "added")

    def test_group_edited(self):
        self.planted("the callee changed", "(*state)=Client(", "(*state)=Configure(", "body differs")

    def test_write_lost(self):
        self.planted("the write to state dropped", "(*state)=Leave(windows,event,(*state));",
                     "Leave(windows,event,(*state));", "body differs")

    def test_transposed_argument(self):
        self.planted("link passes windows for display",
                     "RespondB(display,windows,event,targets);",
                     "RespondB(windows,display,event,targets);", "passes")

    def test_link_dropped(self):
        self.planted("default does not call the next helper",
                     "      RespondB(display,windows,event,targets);\n",
                     "", "lost")

    def test_default_moved(self):
        self.planted("default body edited in the innermost helper",
                     "    default:\n      break;\n  }\n}\n\nstatic void RespondA",
                     "    default:\n    {\n      (*state)=0;\n      break;\n    }\n  }\n}\n\nstatic void RespondA",
                     "body differs")

    def test_kept_group_edited(self):
        self.planted("kept group changed", "Press(display,windows,event,&targets);",
                     "Press(display,windows,event,targets);", "kept group")

    def test_statement_added_to_caller(self):
        self.planted("extra call in the caller", "  Finish(state);\n}", "  Finish(state);\n  Extra();\n}",
                     "outside the helpers")

    def test_other_text_changed(self):
        self.planted("typedef member renamed", "*dismiss;", "*dismissed;", "outside the helpers")

    def test_wrong_binding(self):
        self.planted("state bound from another member",
                     "*const state = targets->state;",
                     "*const state = targets->dismiss;", "bound from member")

    def test_initialiser_out_of_order(self):
        self.planted("targets initialiser swapped", "{ &dismiss, &state }", "{ &state, &dismiss }",
                     "initialiser")

    def test_return_in_moved_group(self):
        self.planted("a return inside a moved group",
                     "(*state)=Client(display,windows,event,(*state));\n      break;",
                     "(*state)=Client(display,windows,event,(*state));\n      return;", "return")

    def test_reordered_groups(self):
        a = """    case ClientMessage:
    {
      (*state)=Client(display,windows,event,(*state));
      break;
    }
    case ConfigureNotify:
    {
      (*state)=Configure(windows,event,(*state));
      break;
    }
"""
        b = """    case ConfigureNotify:
    {
      (*state)=Configure(windows,event,(*state));
      break;
    }
    case ClientMessage:
    {
      (*state)=Client(display,windows,event,(*state));
      break;
    }
"""
        self.planted("two groups swapped", a, b, "original order")

    def test_extra_function(self):
        self.refused(OLD, NEW + "\nstatic void Stray(void)\n{\n}\n", ["RespondA", "RespondB"],
                     "functions added")

    def test_other_controlling_expression(self):
        self.planted("helper switches on another expression",
                     "switch (event.type)\n  {\n    case KeyPress:", "switch (event_type)\n  {\n    case KeyPress:",
                     "controls on")


CTX_OLD = """
typedef struct _Context
{
  Display
    *display;

  Image
    **image;
} Context;

static Image *Caller(Display *display,Image **image,const Command command)
{
  Image
    *nexus;

  MagickStatusType
    flags;

  QuantizeInfo
    quantize_info;

  Context
    context;

  nexus=NewImageList();
  GetQuantizeInfo(&quantize_info);
  context.display=display;
  context.image=image;
  switch (command)
  {
    case OpenCommand:
      nexus=Open(&context);
      break;
    case MapCommand:
      Map(&context,quantize_info);
      break;
    case BlurCommand:
      Blur(&context,&flags);
      break;
    case NewCommand:
      nexus=New(&context,nexus);
      break;
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
  return(nexus);
}
"""

CTX_NEW = """
typedef struct _Context
{
  Display
    *display;

  Image
    **image;

  QuantizeInfo
    *quantize_info;
} Context;

static void Rest(const Context *context,const Command command,
  MagickStatusType *flags,Image **nexus)
{
  Display
    *const display = context->display;

  QuantizeInfo
    *const quantize_info = context->quantize_info;

  switch (command)
  {
    case MapCommand:
      Map(context,(*quantize_info));
      break;
    case BlurCommand:
      Blur(context,&(*flags));
      break;
    case NewCommand:
      (*nexus)=New(context,(*nexus));
      break;
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static Image *Caller(Display *display,Image **image,const Command command)
{
  Image
    *nexus;

  MagickStatusType
    flags;

  QuantizeInfo
    quantize_info;

  Context
    context;

  nexus=NewImageList();
  GetQuantizeInfo(&quantize_info);
  context.display=display;
  context.image=image;
  context.quantize_info=&quantize_info;
  switch (command)
  {
    case OpenCommand:
      nexus=Open(&context);
      break;
    default:
    {
      Rest(&context,command,&flags,&nexus);
      break;
    }
  }
  return(nexus);
}
"""


class FilledContextCheck(unittest.TestCase):
    def problems(self, new):
        return S.check(CTX_OLD, new, "Caller", ["Rest"])[0]

    def test_filled_context_step(self):
        self.assertEqual(self.problems(CTX_NEW), [])

    def planted(self, what, was, now, text):
        self.assertIn(was, CTX_NEW, what)
        problems = self.problems(CTX_NEW.replace(was, now, 1))
        self.assertTrue(any(text in p for p in problems), (what, text, problems))

    def test_member_by_address_without_deref(self):
        self.planted("quantize_info filled by address, passed as the pointer",
                     "Map(context,(*quantize_info));", "Map(context,quantize_info);",
                     "without (*quantize_info)")

    def test_member_by_value_dereferenced(self):
        self.planted("quantize_info filled by value, used as (*v)",
                     "context.quantize_info=&quantize_info;",
                     "context.quantize_info=quantize_info;", "filled by value")

    def test_bound_member_not_filled(self):
        self.planted("display bound, its fill removed",
                     "  context.display=display;\n", "", "does not fill it")

    def test_appended_member_not_filled(self):
        self.planted("the new member's fill dropped",
                     "  context.quantize_info=&quantize_info;\n", "", "does not fill it")

    def test_appended_fill_out_of_place(self):
        self.planted("the new fill moved before the existing ones",
                     "  context.display=display;\n  context.image=image;\n"
                     "  context.quantize_info=&quantize_info;\n",
                     "  context.quantize_info=&quantize_info;\n  context.display=display;\n"
                     "  context.image=image;\n", "existing fills")

    def test_appended_fill_not_adjacent(self):
        self.planted("a statement between the fills",
                     "  context.image=image;\n  context.quantize_info=&quantize_info;\n",
                     "  context.image=image;\n  Extra();\n"
                     "  context.quantize_info=&quantize_info;\n", "outside the helpers")

    def test_existing_member_changed(self):
        self.planted("an existing member's type changed", "  Image\n    **image;\n\n  Quantize",
                     "  Image\n    *image;\n\n  Quantize", "existing members changed")

    def test_member_inserted_not_appended(self):
        self.planted("the new member inserted first",
                     "{\n  Display\n    *display;\n\n  Image\n    **image;\n\n  QuantizeInfo\n"
                     "    *quantize_info;\n}",
                     "{\n  QuantizeInfo\n    *quantize_info;\n\n  Display\n    *display;\n\n"
                     "  Image\n    **image;\n}", "not only appended")

    def test_written_group_edited(self):
        self.planted("New's result discarded", "(*nexus)=New(context,(*nexus));",
                     "(void) New(context,(*nexus));", "body differs")

    def test_written_pointer_transposed(self):
        self.planted("flags and nexus swapped in the call", "Rest(&context,command,&flags,&nexus);",
                     "Rest(&context,command,&nexus,&flags);", "where the parameter is")


if __name__ == "__main__":
    unittest.main()
