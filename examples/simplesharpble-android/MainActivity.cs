using Android;
using Android.Content;
using Android.Content.PM;
using ColorStateList = Android.Content.Res.ColorStateList;
using Android.Graphics;
using Android.Graphics.Drawables;
using Android.Views;
using Java.Interop;
using SimpleSharpBLE;

namespace SimpleSharpBLEScan;

[Activity(Label = "SimpleSharpBLE Explorer", MainLauncher = true, Exported = true,
    Theme = "@style/ExplorerTheme", ConfigurationChanges = ConfigChanges.Orientation | ConfigChanges.ScreenSize)]
public class MainActivity : Activity
{
    private static readonly string[] permissions =
        [Manifest.Permission.BluetoothScan, Manifest.Permission.BluetoothConnect];
    private sealed record Device(string Name, string Address, int Rssi);
    private List<Device> discovered = [];
    private TextView adapterName = null!, adapterAddress = null!, status = null!, adapterState = null!, count = null!;
    private Button scan = null!, refresh = null!, enableBluetooth = null!;
    private EditText filter = null!;
    private LinearLayout results = null!;
    private bool busy, initialized, permissionRequested;

    protected override void OnCreate(Bundle? state)
    {
        base.OnCreate(state);
        permissionRequested = state?.GetBoolean("permissionRequested") ?? false;
    }

    protected override void OnResume()
    {
        base.OnResume();
        if (!busy) ShowScreen();
    }

    protected override void OnSaveInstanceState(Bundle state)
    {
        state.PutBoolean("permissionRequested", permissionRequested);
        base.OnSaveInstanceState(state);
    }

    public override void OnRequestPermissionsResult(int requestCode, string[] requested, Permission[] granted)
    {
        base.OnRequestPermissionsResult(requestCode, requested, granted);
        ShowScreen();
    }

    private void ShowScreen()
    {
        var root = Column();
        root.SetFitsSystemWindows(true);
        root.SetBackgroundColor(ColorOf(Resource.Color.background));
        SetContentView(root);
        var scroll = new ScrollView(this) { FillViewport = true };
        var body = Column(20);
        scroll.AddView(body);

        if (permissions.Any(permission => CheckSelfPermission(permission) != Permission.Granted))
        {
            root.AddView(scroll, new LinearLayout.LayoutParams(-1, -1));
            body.SetGravity(GravityFlags.CenterVertical);
            var badge = Text("BLE", 28, Resource.Color.on_primary_container, bold: true);
            badge.Gravity = GravityFlags.Center;
            badge.Background = Shape(Resource.Color.primary_container, 44);
            body.AddView(badge, new LinearLayout.LayoutParams(Dp(88), Dp(88)) { BottomMargin = Dp(24) });
            body.AddView(Text("Find nearby Bluetooth devices", 24, bold: true));
            body.AddView(Text("SimpleSharpBLE Explorer needs Nearby devices permission to scan. Device data stays on this phone.", 16, Resource.Color.muted));
            AddSpaced(body, ActionButton(permissionRequested ? "Try again" : "Continue", () =>
            {
                permissionRequested = true;
                RequestPermissions(permissions, 1);
            }));
            if (permissionRequested)
                AddSpaced(body, ActionButton("Open app settings", () => StartActivity(new Intent(
                    Android.Provider.Settings.ActionApplicationDetailsSettings,
                    Android.Net.Uri.FromParts("package", PackageName, null))), secondary: true));
            return;
        }

        var header = new LinearLayout(this);
        header.SetGravity(GravityFlags.Center);
        header.SetBackgroundColor(ColorOf(Resource.Color.surface));
        var logo = new ImageView(this) { ContentDescription = "SimpleBLE" };
        logo.SetImageResource(Resource.Drawable.simpleble_logo);
        logo.SetScaleType(ImageView.ScaleType.FitCenter);
        header.AddView(logo, new LinearLayout.LayoutParams(Dp(130), Dp(44)));
        header.AddView(Text("  Explorer", 14, Resource.Color.muted));
        root.AddView(header, new LinearLayout.LayoutParams(-1, Dp(72)));
        root.AddView(scroll, new LinearLayout.LayoutParams(-1, 0, 1));

        var card = Column(20);
        card.Background = Shape(Resource.Color.primary_container, 24);
        card.AddView(Text("Bluetooth adapter", 14, Resource.Color.on_primary_container));
        adapterName = Text("", 18, Resource.Color.on_primary_container, bold: true);
        card.AddView(adapterName);
        adapterAddress = Text("", 12, Resource.Color.on_primary_container);
        adapterAddress.Typeface = Typeface.Monospace;
        card.AddView(adapterAddress);
        adapterState = Text("", 14, Resource.Color.on_primary_container, bold: true);
        card.AddView(adapterState);
        status = Text("", 14, Resource.Color.on_primary_container);
        card.AddView(status);
        refresh = ActionButton("Refresh", RefreshAdapter, secondary: true);
        AddSpaced(card, refresh);
        enableBluetooth = ActionButton("Turn on Bluetooth", () =>
            StartActivity(new Intent(Android.Bluetooth.BluetoothAdapter.ActionRequestEnable)));
        AddSpaced(card, enableBluetooth);
        body.AddView(card);

        var heading = new LinearLayout(this);
        heading.SetGravity(GravityFlags.CenterVertical);
        var titles = Column();
        titles.AddView(Text("Nearby devices", 22, bold: true));
        count = Text("", 14, Resource.Color.muted);
        titles.AddView(count);
        heading.AddView(titles, new LinearLayout.LayoutParams(0, -2, 1));
        scan = ActionButton("Scan", async () => await ScanAsync());
        heading.AddView(scan);
        AddSpaced(body, heading);
        filter = new EditText(this) { Hint = "Filter by name or address", TextSize = 16 };
        filter.SetSingleLine(true);
        filter.TextChanged += (_, _) => RenderDevices();
        AddSpaced(body, filter);
        results = Column();
        body.AddView(results);
        RefreshAdapter();
        RenderDevices();
    }

    private void RefreshAdapter()
    {
        if (busy) return;
        scan.Enabled = false;
        enableBluetooth.Visibility = ViewStates.Gone;
        try
        {
            if (!initialized)
            {
                // The native backend captures its scanner during initialization.
                var manager = GetSystemService(BluetoothService) as Android.Bluetooth.BluetoothManager;
                var systemAdapter = manager?.Adapter;
                if (systemAdapter?.IsEnabled != true)
                {
                    adapterName.Text = systemAdapter?.Name ?? "Not available";
                    adapterState.Text = "Off";
                    status.Text = systemAdapter is null ? "No Bluetooth adapter found." : "Turn on Bluetooth to scan.";
                    enableBluetooth.Visibility = systemAdapter is null ? ViewStates.Gone : ViewStates.Visible;
                    return;
                }
                // Preload the Java bridge on the application thread before worker calls.
                Java.Lang.JavaSystem.LoadLibrary("simpleble");
                Advanced.Android.Initialize(JniEnvironment.Runtime.InvocationPointer, Application.Context.Handle);
                initialized = true;
            }
            var adapters = SimpleSharpBLE.Adapter.GetAdapters();
            try
            {
                var adapter = adapters.FirstOrDefault();
                adapterName.Text = adapter?.Identifier ?? "Not available";
                adapterAddress.Text = adapter?.Address ?? "";
                bool ready = adapter?.IsPowered == true;
                adapterState.Text = ready ? "Ready" : "Off";
                status.Text = ready ? "Ready to scan for nearby devices." : "Enable Bluetooth, then refresh.";
                enableBluetooth.Visibility = adapter is not null && !ready ? ViewStates.Visible : ViewStates.Gone;
                scan.Enabled = ready;
            }
            finally { foreach (var adapter in adapters) adapter.Dispose(); }
        }
        catch (Exception error) { status.Text = error.Message; }
    }

    private async Task ScanAsync()
    {
        busy = true;
        scan.Enabled = false;
        refresh.Enabled = false;
        scan.Text = "Scanning…";
        status.Text = "Scanning for five seconds…";
        discovered.Clear();
        RenderDevices();
        try
        {
            var found = await Task.Run(() =>
            {
                var adapters = SimpleSharpBLE.Adapter.GetAdapters();
                try
                {
                    var adapter = adapters.FirstOrDefault() ?? throw new InvalidOperationException("No Bluetooth adapter found.");
                    adapter.ScanFor(TimeSpan.FromSeconds(5));
                    var peripherals = adapter.ScanGetResults();
                    try { return peripherals.Select(p => new Device(p.Identifier, p.Address, p.Rssi)).ToList(); }
                    finally { foreach (var peripheral in peripherals) peripheral.Dispose(); }
                }
                finally { foreach (var adapter in adapters) adapter.Dispose(); }
            });
            if (IsDestroyed) return;
            discovered = found;
            status.Text = "Scan complete.";
        }
        catch (Exception error) { if (!IsDestroyed) status.Text = error.Message; }
        finally
        {
            busy = false;
            if (!IsDestroyed)
            {
                scan.Enabled = true;
                refresh.Enabled = true;
                scan.Text = "Scan";
                RenderDevices();
            }
        }
    }

    private void RenderDevices()
    {
        results.RemoveAllViews();
        count.Text = busy ? "Listening for advertisements" : $"{discovered.Count} discovered";
        filter.Visibility = discovered.Count > 0 ? ViewStates.Visible : ViewStates.Gone;
        string query = filter.Text?.Trim() ?? "";
        var visible = discovered.Where(d => d.Name.Contains(query, StringComparison.OrdinalIgnoreCase) ||
            d.Address.Contains(query, StringComparison.OrdinalIgnoreCase)).ToList();
        if (visible.Count == 0)
        {
            var empty = Column(24);
            empty.Background = Shape(Resource.Color.surface_variant, 20);
            empty.AddView(Text(busy ? "Scanning nearby…" : query.Length > 0 ? "No matching devices" : "No devices yet", 16, bold: true));
            empty.AddView(Text(busy ? "Keep the peripheral awake and nearby. Results appear when the scan finishes." :
                query.Length > 0 ? "Try a different name or Bluetooth address." : "Start a scan to discover nearby BLE peripherals.", 14, Resource.Color.muted));
            AddSpaced(results, empty);
        }
        foreach (var device in visible)
        {
            var row = Column(18);
            row.Background = Shape(Resource.Color.surface, 20, border: true);
            row.AddView(Text(string.IsNullOrWhiteSpace(device.Name) ? "Unnamed device" : device.Name, 16, bold: true));
            var address = Text(device.Address, 12, Resource.Color.muted);
            address.Typeface = Typeface.Monospace;
            row.AddView(address);
            row.AddView(Text($"{device.Rssi} dBm", 14, Resource.Color.primary, bold: true));
            AddSpaced(results, row);
        }
    }

    private Color ColorOf(int resource) => new(GetColor(resource));
    private int Dp(int value) => (int)(value * Resources!.DisplayMetrics!.Density + 0.5f);
    private LinearLayout Column(int padding = 0)
    {
        var column = new LinearLayout(this) { Orientation = Orientation.Vertical };
        column.SetPadding(Dp(padding), Dp(padding), Dp(padding), Dp(padding));
        return column;
    }
    private TextView Text(string value, int size, int color = Resource.Color.foreground, bool bold = false)
    {
        var text = new TextView(this) { Text = value, TextSize = size };
        text.SetTextColor(ColorOf(color));
        if (bold) text.SetTypeface(Typeface.Default, TypefaceStyle.Bold);
        text.SetPadding(0, Dp(3), 0, Dp(3));
        return text;
    }
    private GradientDrawable Shape(int color, int radius, bool border = false)
    {
        var shape = new GradientDrawable();
        shape.SetColor(ColorOf(color));
        shape.SetCornerRadius(Dp(radius));
        if (border) shape.SetStroke(Dp(1), ColorOf(Resource.Color.outline));
        return shape;
    }
    private Button ActionButton(string title, Action action, bool secondary = false)
    {
        var button = new Button(this) { Text = title, TextSize = 14 };
        button.SetAllCaps(false);
        button.SetTextColor(ColorOf(secondary ? Resource.Color.foreground : Resource.Color.on_primary));
        button.Background = new RippleDrawable(ColorStateList.ValueOf(ColorOf(Resource.Color.outline)),
            Shape(secondary ? Resource.Color.surface_variant : Resource.Color.primary, 24), null);
        button.SetPadding(Dp(20), 0, Dp(20), 0);
        button.SetMinimumHeight(Dp(48));
        button.Click += (_, _) => action();
        return button;
    }
    private void AddSpaced(LinearLayout parent, View view) =>
        parent.AddView(view, new LinearLayout.LayoutParams(-1, -2) { TopMargin = Dp(14) });
}
