#include "bot/bot_controller.h"
#include "driver/keyboard.h"
#include "driver/screen.h"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <linux/input.h>
#include <string>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

/* ============================================================
 * Global state
 * ============================================================ */

static BotController bot;
static std::atomic<bool> bot_active(false);

// GUI widgets
static GtkWidget *toggle_btn;
static GtkWidget *auto_calibrate_btn;
static GtkWidget *calibrate_btn;
static GtkWidget *status_label;
static GtkWidget *fen_label;
static GtkWidget *eval_label;
static GtkWidget *last_move_label;
static GtkWidget *depth_scale;
static GtkWidget *delay_min_spin;
static GtkWidget *delay_max_spin;
static GtkWidget *color_combo;

// Calibration state
static std::atomic<int> calib_state(0); // 0=idle, 1=waiting_first, 2=waiting_second
static int calib_x1, calib_y1;

/* ============================================================
 * Test modes (--test-stockfish, --test-driver)
 * ============================================================ */

static int test_stockfish()
{
  printf("=== Stockfish Communication Test ===\n");

  StockfishEngine sf;

  if (!sf.start())
  {
    fprintf(stderr, "FAIL: Could not start Stockfish\n");
    return 1;
  }

  printf("OK: Stockfish started: %s\n", sf.get_engine_info().c_str());

  // Test with starting position
  sf.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  std::string move = sf.get_best_move(10);

  if (move.empty())
  {
    fprintf(stderr, "FAIL: No best move returned\n");
    sf.stop();
    return 1;
  }

  printf("OK: Best move from startpos (depth 10): %s\n", move.c_str());
  printf("OK: Eval: %d cp\n", sf.get_last_score());
  printf("OK: PV: %s\n", sf.get_last_pv().c_str());

  // Test with a tactical position (Italian Game: 1.e4 e5 2.Nf3 Nc6 3.Bc4 Nf6)
  sf.set_position("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
  move = sf.get_best_move(10);
  printf("OK: Italian Game (depth 10): %s (eval: %d cp)\n", move.c_str(), sf.get_last_score());

  sf.stop();
  printf("=== All Stockfish tests PASSED ===\n");
  return 0;
}

static int test_driver()
{
  printf("=== Kernel Driver Test ===\n");

  // Test mouse
  RkkdrMouse mouse;
  if (!mouse.open())
  {
    fprintf(stderr, "FAIL: Could not open /dev/rkkdr_mouse\n");
    fprintf(stderr, "      Is the RKKDR driver loaded? (make load in KernelDriver/)\n");
    return 1;
  }
  printf("OK: /dev/rkkdr_mouse opened\n");

  // Test keyboard
  RkkdrKeyboard kbd;
  if (!kbd.open())
  {
    fprintf(stderr, "FAIL: Could not open /dev/rkkdr_keyboard\n");
    return 1;
  }
  printf("OK: /dev/rkkdr_keyboard opened\n");

  // Test screen info
  RkkdrScreen screen;
  if (!screen.open())
  {
    fprintf(stderr, "WARN: Could not open /dev/rkkdr_screen (non-fatal)\n");
  }
  else
  {
    printf("OK: /dev/rkkdr_screen opened\n");
    printf("OK: Display: %dx%d\n", screen.get_width(), screen.get_height());
  }

  // Test mouse movement (move to center of screen)
  int cx = screen.get_width() / 2;
  int cy = screen.get_height() / 2;
  printf("OK: Moving mouse to center (%d, %d)...\n", cx, cy);
  mouse.move_to(cx, cy);

  printf("=== All driver tests PASSED ===\n");
  return 0;
}

/* ============================================================
 * GUI update helpers (thread-safe via g_idle_add)
 * ============================================================ */

struct StatusUpdate
{
  std::string text;
};

static gboolean update_status_idle(gpointer data)
{
  StatusUpdate *upd = (StatusUpdate *)data;
  if (status_label)
    gtk_label_set_text(GTK_LABEL(status_label), upd->text.c_str());
  delete upd;
  return G_SOURCE_REMOVE;
}

static gboolean update_fen_idle(gpointer data)
{
  StatusUpdate *upd = (StatusUpdate *)data;
  if (fen_label)
    gtk_label_set_text(GTK_LABEL(fen_label), upd->text.c_str());
  delete upd;
  return G_SOURCE_REMOVE;
}

static gboolean update_move_idle(gpointer data)
{
  StatusUpdate *upd = (StatusUpdate *)data;
  if (last_move_label)
    gtk_label_set_text(GTK_LABEL(last_move_label), upd->text.c_str());

  // Also update eval
  if (eval_label)
    gtk_label_set_text(GTK_LABEL(eval_label), bot.get_engine_eval().c_str());

  delete upd;
  return G_SOURCE_REMOVE;
}

static gboolean update_toggle_btn_idle(gpointer data)
{
  bool active = bot_active.load();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_btn), active);
  return G_SOURCE_REMOVE;
}

/* ============================================================
 * GUI callbacks
 * ============================================================ */

static void on_toggle(GtkWidget *widget, gpointer data)
{
  bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  bot_active = active;

  if (active)
  {
    gtk_button_set_label(GTK_BUTTON(widget), "⏹ Stop Bot");
    bot.start();
  }
  else
  {
    gtk_button_set_label(GTK_BUTTON(widget), "▶ Start Bot");
    bot.stop();
  }
}

static void on_auto_calibrate_clicked(GtkWidget *widget, gpointer data)
{
  bot.auto_calibrate();
}

static void on_calibrate_clicked(GtkWidget *widget, gpointer data)
{
  if (calib_state == 0)
  {
    calib_state = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Click TOP-LEFT corner (a8)...");
    gtk_label_set_text(GTK_LABEL(status_label), "CALIBRATING: Click the TOP-LEFT corner of the board (a8 square)");
  }
}

static void on_depth_changed(GtkRange *range, gpointer data)
{
  int depth = (int)gtk_range_get_value(range);
  bot.set_stockfish_depth(depth);
}

static void on_color_changed(GtkComboBox *combo, gpointer data)
{
  int color_idx = gtk_combo_box_get_active(combo);
  bot.set_playing_white(color_idx == 0);
}

static void on_delay_changed(GtkSpinButton *spin, gpointer data)
{
  int delay_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(delay_min_spin));
  int delay_max = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(delay_max_spin));
  bot.set_move_delay(delay_min, delay_max);
}

/* ============================================================
 * Hotkey + Calibration click listener thread
 * ============================================================ */

static void input_listener_thread()
{
  int epfd = epoll_create1(0);
  if (epfd < 0)
    return;

  std::vector<int> fds;
  DIR *dir = opendir("/dev/input");
  if (dir)
  {
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
      std::string name = ent->d_name;
      if (name.find("event") == 0)
      {
        std::string path = "/dev/input/" + name;
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0)
        {
          unsigned long evbit[1 + EV_MAX / 8 / sizeof(long)] = {0};
          ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit);
          // Listen for both keyboard and mouse events
          if (evbit[0] & ((1UL << EV_KEY) | (1UL << EV_ABS) | (1UL << EV_REL)))
          {
            fds.push_back(fd);
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fd;
            epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
          }
          else
          {
            close(fd);
          }
        }
      }
    }
    closedir(dir);
  }

  struct epoll_event events[10];
  while (true)
  {
    int n = epoll_wait(epfd, events, 10, -1);
    for (int i = 0; i < n; i++)
    {
      struct input_event ev;
      while (read(events[i].data.fd, &ev, sizeof(ev)) > 0)
      {
        // Hotkey: backtick (`) to toggle bot
        if (ev.type == EV_KEY && ev.code == KEY_GRAVE && ev.value == 1)
        {
          bool current = bot_active.load();
          bot_active = !current;

          if (bot_active)
            bot.start();
          else
            bot.stop();

          g_idle_add(update_toggle_btn_idle, NULL);
        }

        // Calibration click detection (real mouse click, not virtual)
        if (ev.type == EV_KEY && ev.code == BTN_LEFT && ev.value == 1)
        {
          if (calib_state == 1 || calib_state == 2)
          {
            // Read current cursor position from X11
            // We'll use xdotool-style query via XQueryPointer
            Display *dpy = XOpenDisplay(nullptr);
            if (dpy)
            {
              Window root = DefaultRootWindow(dpy);
              Window child;
              int root_x, root_y, win_x, win_y;
              unsigned int mask;
              XQueryPointer(dpy, root, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
              XCloseDisplay(dpy);

              if (calib_state == 1)
              {
                calib_x1 = root_x;
                calib_y1 = root_y;
                calib_state = 2;

                auto *upd = new StatusUpdate{"CALIBRATING: Now click BOTTOM-RIGHT corner (h1)"};
                g_idle_add(update_status_idle, upd);
              }
              else if (calib_state == 2)
              {
                bot.calibrate(calib_x1, calib_y1, root_x, root_y);
                calib_state = 0;

                auto *upd = new StatusUpdate{"Board calibrated! Ready to start."};
                g_idle_add(update_status_idle, upd);

                // Reset calibrate button label
                g_idle_add(
                    +[](gpointer data) -> gboolean
                    {
                      gtk_button_set_label(GTK_BUTTON(calibrate_btn), "🎯 Calibrate Board");
                      return G_SOURCE_REMOVE;
                    },
                    NULL
                );
              }
            }
          }
        }
      }
    }
  }

  for (int fd : fds)
    close(fd);
  close(epfd);
}

/* ============================================================
 * GTK Application
 * ============================================================ */

static void apply_css(GtkWidget *widget)
{
  GtkCssProvider *provider = gtk_css_provider_new();

  const char *css = "window {"
                    "  background-color: #1a1a2e;"
                    "}"
                    "label {"
                    "  color: #e0e0e0;"
                    "  font-family: 'JetBrains Mono', 'Fira Code', monospace;"
                    "}"
                    "label.title {"
                    "  font-size: 18px;"
                    "  font-weight: bold;"
                    "  color: #00d4ff;"
                    "}"
                    "label.section {"
                    "  font-size: 13px;"
                    "  font-weight: bold;"
                    "  color: #7b68ee;"
                    "}"
                    "label.status {"
                    "  font-size: 12px;"
                    "  color: #ffd700;"
                    "}"
                    "label.mono {"
                    "  font-size: 11px;"
                    "  color: #b0b0b0;"
                    "}"
                    "button, togglebutton {"
                    "  background: linear-gradient(180deg, #2d2d5e, #1e1e3f);"
                    "  color: #e0e0e0;"
                    "  border: 1px solid #444;"
                    "  border-radius: 6px;"
                    "  padding: 8px 16px;"
                    "  font-family: 'JetBrains Mono', monospace;"
                    "  font-weight: bold;"
                    "}"
                    "button:hover {"
                    "  background: linear-gradient(180deg, #3d3d7e, #2e2e5f);"
                    "  border-color: #00d4ff;"
                    "}"
                    "button.toggle-on {"
                    "  background: linear-gradient(180deg, #cc3333, #991111);"
                    "  border-color: #ff4444;"
                    "}"
                    "scale trough {"
                    "  background-color: #333;"
                    "  border-radius: 4px;"
                    "}"
                    "scale highlight {"
                    "  background-color: #00d4ff;"
                    "  border-radius: 4px;"
                    "}"
                    "scale slider {"
                    "  background-color: #e0e0e0;"
                    "  border-radius: 50%;"
                    "}"
                    "combobox button {"
                    "  background: #2d2d5e;"
                    "  color: #e0e0e0;"
                    "}"
                    "spinbutton {"
                    "  background: #2d2d5e;"
                    "  color: #e0e0e0;"
                    "  border: 1px solid #444;"
                    "  border-radius: 4px;"
                    "}"
                    "separator {"
                    "  background-color: #333366;"
                    "  min-height: 1px;"
                    "}";

  gtk_css_provider_load_from_data(provider, css, -1, NULL);
  gtk_style_context_add_provider_for_screen(
      gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
  g_object_unref(provider);
}

static GtkWidget *create_label(const char *text, const char *css_class)
{
  GtkWidget *label = gtk_label_new(text);
  gtk_widget_set_halign(label, GTK_ALIGN_START);

  if (css_class)
  {
    GtkStyleContext *ctx = gtk_widget_get_style_context(label);
    gtk_style_context_add_class(ctx, css_class);
  }

  return label;
}

static void activate(GtkApplication *app, gpointer user_data)
{
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "ChessBot — Chess.com Automation");
  gtk_window_set_default_size(GTK_WINDOW(window), 420, 560);
  gtk_container_set_border_width(GTK_CONTAINER(window), 16);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

  apply_css(window);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  // ── Title ──
  GtkWidget *title = create_label("♟ ChessBot", "title");
  gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);

  GtkWidget *subtitle = create_label("Chess.com Automation • Kernel Driver + Stockfish", "mono");
  gtk_widget_set_halign(subtitle, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(vbox), subtitle, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

  // ── Auto-Detect / Calibration ──
  auto_calibrate_btn = gtk_button_new_with_label("✨ Auto-Detect Board");
  g_signal_connect(auto_calibrate_btn, "clicked", G_CALLBACK(on_auto_calibrate_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), auto_calibrate_btn, FALSE, FALSE, 0);

  calibrate_btn = gtk_button_new_with_label("🎯 Manual Calibration");
  g_signal_connect(calibrate_btn, "clicked", G_CALLBACK(on_calibrate_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), calibrate_btn, FALSE, FALSE, 0);

  // ── Color Selector ──
  GtkWidget *color_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start(GTK_BOX(vbox), color_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(color_hbox), create_label("Playing as:", NULL), FALSE, FALSE, 0);

  color_combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "White");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(color_combo), "Black");
  gtk_combo_box_set_active(GTK_COMBO_BOX(color_combo), 0);
  g_signal_connect(color_combo, "changed", G_CALLBACK(on_color_changed), NULL);
  gtk_box_pack_start(GTK_BOX(color_hbox), color_combo, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

  // ── Engine Settings ──
  gtk_box_pack_start(GTK_BOX(vbox), create_label("Engine Settings", "section"), FALSE, FALSE, 0);

  // Depth slider
  GtkWidget *depth_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start(GTK_BOX(vbox), depth_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(depth_hbox), create_label("Depth:", NULL), FALSE, FALSE, 0);

  GtkAdjustment *depth_adj = gtk_adjustment_new(20.0, 1.0, 30.0, 1.0, 5.0, 0.0);
  depth_scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, depth_adj);
  gtk_scale_set_digits(GTK_SCALE(depth_scale), 0);
  gtk_scale_set_value_pos(GTK_SCALE(depth_scale), GTK_POS_RIGHT);
  gtk_widget_set_hexpand(depth_scale, TRUE);
  g_signal_connect(depth_scale, "value-changed", G_CALLBACK(on_depth_changed), NULL);
  gtk_box_pack_start(GTK_BOX(depth_hbox), depth_scale, TRUE, TRUE, 0);

  // Move delay
  GtkWidget *delay_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start(GTK_BOX(vbox), delay_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(delay_hbox), create_label("Delay (ms):", NULL), FALSE, FALSE, 0);

  GtkAdjustment *delay_min_adj = gtk_adjustment_new(200.0, 50.0, 5000.0, 50.0, 100.0, 0.0);
  delay_min_spin = gtk_spin_button_new(delay_min_adj, 50.0, 0);
  g_signal_connect(delay_min_spin, "value-changed", G_CALLBACK(on_delay_changed), NULL);
  gtk_box_pack_start(GTK_BOX(delay_hbox), delay_min_spin, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(delay_hbox), create_label("-", NULL), FALSE, FALSE, 0);

  GtkAdjustment *delay_max_adj = gtk_adjustment_new(500.0, 50.0, 5000.0, 50.0, 100.0, 0.0);
  delay_max_spin = gtk_spin_button_new(delay_max_adj, 50.0, 0);
  g_signal_connect(delay_max_spin, "value-changed", G_CALLBACK(on_delay_changed), NULL);
  gtk_box_pack_start(GTK_BOX(delay_hbox), delay_max_spin, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

  // ── Start / Stop ──
  toggle_btn = gtk_toggle_button_new_with_label("▶ Start Bot");
  g_signal_connect(toggle_btn, "toggled", G_CALLBACK(on_toggle), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), toggle_btn, FALSE, FALSE, 0);

  // Initialize bot settings from default GUI values
  on_color_changed(GTK_COMBO_BOX(color_combo), NULL);
  on_depth_changed(GTK_RANGE(depth_scale), NULL);
  on_delay_changed(GTK_SPIN_BUTTON(delay_min_spin), NULL);

  GtkWidget *hotkey_hint = create_label("Hotkey: ` (backtick) to toggle", "mono");
  gtk_widget_set_halign(hotkey_hint, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(vbox), hotkey_hint, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

  // ── Status Display ──
  gtk_box_pack_start(GTK_BOX(vbox), create_label("Status", "section"), FALSE, FALSE, 0);

  status_label = create_label("Initializing...", "status");
  gtk_label_set_line_wrap(GTK_LABEL(status_label), TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

  // FEN
  GtkWidget *fen_title = create_label("FEN:", "mono");
  gtk_box_pack_start(GTK_BOX(vbox), fen_title, FALSE, FALSE, 0);

  fen_label = create_label("—", "mono");
  gtk_label_set_line_wrap(GTK_LABEL(fen_label), TRUE);
  gtk_label_set_selectable(GTK_LABEL(fen_label), TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), fen_label, FALSE, FALSE, 0);

  // Last move + Eval
  GtkWidget *info_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_box_pack_start(GTK_BOX(vbox), info_hbox, FALSE, FALSE, 0);

  GtkWidget *move_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start(GTK_BOX(info_hbox), move_vbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(move_vbox), create_label("Last Move:", "mono"), FALSE, FALSE, 0);
  last_move_label = create_label("—", "status");
  gtk_box_pack_start(GTK_BOX(move_vbox), last_move_label, FALSE, FALSE, 0);

  GtkWidget *eval_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start(GTK_BOX(info_hbox), eval_vbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(eval_vbox), create_label("Eval:", "mono"), FALSE, FALSE, 0);
  eval_label = create_label("—", "status");
  gtk_box_pack_start(GTK_BOX(eval_vbox), eval_label, FALSE, FALSE, 0);

  gtk_widget_show_all(window);

  // Set up bot callbacks for GUI updates
  bot.on_status_change = [](const std::string &status)
  {
    auto *upd = new StatusUpdate{status};
    g_idle_add(update_status_idle, upd);
  };

  bot.on_fen_update = [](const std::string &fen)
  {
    auto *upd = new StatusUpdate{fen};
    g_idle_add(update_fen_idle, upd);
  };

  bot.on_move_made = [](const std::string &move)
  {
    auto *upd = new StatusUpdate{move};
    g_idle_add(update_move_idle, upd);
  };

  // Initialize bot subsystems
  if (!bot.init())
  {
    gtk_label_set_text(GTK_LABEL(status_label), "INIT FAILED — check console for errors");
  }
}

/* ============================================================
 * Main
 * ============================================================ */

#include <X11/Xlib.h>

int main(int argc, char **argv)
{
  XInitThreads();

  // Ignore SIGPIPE to prevent crash when Stockfish pipe closes
  signal(SIGPIPE, SIG_IGN);

  // Handle test modes
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--test-stockfish") == 0)
      return test_stockfish();
    if (strcmp(argv[i], "--test-driver") == 0)
      return test_driver();
    if (strcmp(argv[i], "--help") == 0)
    {
      printf("ChessBot — Chess.com Automation\n\n");
      printf("Usage: %s [OPTIONS]\n\n", argv[0]);
      printf("Options:\n");
      printf("  --test-stockfish   Test Stockfish engine communication\n");
      printf("  --test-driver      Test kernel driver devices\n");
      printf("  --help             Show this help\n\n");
      printf("Hotkey: ` (backtick) to toggle bot on/off\n");
      return 0;
    }
  }

  // Start input listener thread (hotkeys + calibration clicks)
  std::thread input_thread(input_listener_thread);
  input_thread.detach();

  // Launch GTK application
  GtkApplication *app = gtk_application_new("org.riik.chessbot", G_APPLICATION_NON_UNIQUE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  // Cleanup
  bot.cleanup();

  return status;
}
