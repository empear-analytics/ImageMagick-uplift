#!/usr/bin/env python3
"""Tests of tools/switch_split_check.py on a small synthetic file, run from the repository root:

    python3 tools/test_switch_split_check.py

The fixture is the shape of widget.c's `XNoticeWidget`: a `switch` of nine labels and a `default:`
whose groups call helpers that write the caller's `state` through a `const` targets struct. The
correct split, in one step and in two, must print OK; each planted defect, a lost label, a doubled
label, a wrong call, a group edited on the way, a transposed argument, a moved `default:`, a kept group
edited, a statement added to the caller, a binding of the wrong member, a changed initialiser and a
reordering, must be refused.
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

    def planted(self, what, was, now, text, base=NEW, old=OLD, helpers=("RespondA", "RespondB")):
        self.assertIn(was, base, what)
        self.refused(old, base.replace(was, now, 1), list(helpers), text)

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


if __name__ == "__main__":
    unittest.main()
