package org.simpleble.examples.android.ui

import android.app.Activity
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Shapes
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat

private val ExplorerLightColors = lightColorScheme(
    primary = Color(0xFF217FF1),
    onPrimary = Color.White,
    primaryContainer = Color(0xFFE4F0FF),
    onPrimaryContainer = Color(0xFF042853),
    secondary = Color(0xFF536174),
    onSecondary = Color.White,
    secondaryContainer = Color(0xFFF2F5FA),
    onSecondaryContainer = Color(0xFF1F2B38),
    tertiary = Color(0xFF486B98),
    onTertiary = Color.White,
    background = Color(0xFFF8FBFF),
    onBackground = Color(0xFF1F2B38),
    surface = Color.White,
    onSurface = Color(0xFF1F2B38),
    surfaceVariant = Color(0xFFF2F5FA),
    onSurfaceVariant = Color(0xFF6B6B6B),
    outline = Color(0xFFD8E1EC),
    error = Color(0xFFBA1A1A)
)

private val ExplorerDarkColors = darkColorScheme(
    primary = Color(0xFF9DCAFF),
    onPrimary = Color(0xFF003B78),
    primaryContainer = Color(0xFF075BAB),
    onPrimaryContainer = Color(0xFFE4F0FF),
    secondary = Color(0xFFB9C6D9),
    onSecondary = Color(0xFF243247),
    secondaryContainer = Color(0xFF26364E),
    onSecondaryContainer = Color(0xFFDEE8F8),
    tertiary = Color(0xFFAEC9F5),
    onTertiary = Color(0xFF17365C),
    background = Color(0xFF0D1526),
    onBackground = Color(0xFFE6EDF8),
    surface = Color(0xFF111D32),
    onSurface = Color(0xFFE6EDF8),
    surfaceVariant = Color(0xFF26364E),
    onSurfaceVariant = Color(0xFFB9C6D9),
    outline = Color(0xFF8291A8),
    error = Color(0xFFFFB4AB)
)

private val ExplorerTypography = Typography(
    headlineSmall = TextStyle(
        fontSize = 24.sp,
        lineHeight = 30.sp,
        fontWeight = FontWeight.Bold,
        letterSpacing = (-0.4).sp
    ),
    titleLarge = TextStyle(
        fontSize = 22.sp,
        lineHeight = 28.sp,
        fontWeight = FontWeight.Bold,
        letterSpacing = (-0.3).sp
    ),
    titleMedium = TextStyle(
        fontSize = 16.sp,
        lineHeight = 22.sp,
        fontWeight = FontWeight.SemiBold
    ),
    bodyLarge = TextStyle(
        fontSize = 16.sp,
        lineHeight = 24.sp
    ),
    bodyMedium = TextStyle(
        fontSize = 14.sp,
        lineHeight = 20.sp
    ),
    labelLarge = TextStyle(
        fontSize = 14.sp,
        lineHeight = 20.sp,
        fontWeight = FontWeight.SemiBold
    ),
    labelMedium = TextStyle(
        fontSize = 12.sp,
        lineHeight = 16.sp,
        fontWeight = FontWeight.SemiBold,
        letterSpacing = 0.2.sp
    )
)

@Composable
fun SimpleBleExplorerTheme(content: @Composable () -> Unit) {
    val darkTheme = isSystemInDarkTheme()
    val colors = if (darkTheme) ExplorerDarkColors else ExplorerLightColors
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            @Suppress("DEPRECATION")
            window.statusBarColor = colors.background.toArgb()
            @Suppress("DEPRECATION")
            window.navigationBarColor = colors.background.toArgb()
            WindowCompat.getInsetsController(window, view).apply {
                isAppearanceLightStatusBars = !darkTheme
                isAppearanceLightNavigationBars = !darkTheme
            }
        }
    }
    MaterialTheme(
        colorScheme = colors,
        typography = ExplorerTypography,
        shapes = Shapes(
            small = RoundedCornerShape(12.dp),
            medium = RoundedCornerShape(20.dp),
            large = RoundedCornerShape(28.dp)
        ),
        content = content
    )
}
