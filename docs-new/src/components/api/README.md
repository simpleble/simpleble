# API Documentation Components

Componentes React estilizados para mostrar documentación de API de C++ generada desde Doxygen XML.

## Componentes

### `<ApiSection>`

Sección principal que agrupa múltiples clases relacionadas.

**Props:**
- `title: string` - Título de la sección (ej: "Standard API", "Safe API")
- `children: ReactNode` - Contenido de la sección

**Ejemplo:**
```tsx
<ApiSection title="Standard API">
  {/* Clases aquí */}
</ApiSection>
```

### `<ApiClass>`

Componente para mostrar una clase de C++ con su documentación.

**Props:**
- `name: string` - Nombre completo de la clase (ej: "SimpleBLE::Adapter")
- `brief?: string` - Descripción breve de la clase
- `detailed?: string` - Descripción detallada de la clase
- `children?: ReactNode` - Métodos y funciones de la clase

**Ejemplo:**
```tsx
<ApiClass 
  name="SimpleBLE::Adapter"
  brief="Bluetooth Adapter"
  detailed="A default-constructed object is not initialized..."
>
  {/* Métodos aquí */}
</ApiClass>
```

### `<ApiMethod>`

Componente para mostrar un método o función de C++.

**Props:**
- `signature: string` - Firma completa del método (ej: "void scan_start()")
- `brief?: string` - Descripción breve del método
- `detailed?: string` - Descripción detallada del método
- `parameters?: Array<{name: string, type: string, description?: string}>` - Lista de parámetros

**Ejemplo:**
```tsx
<ApiMethod
  signature="void set_callback_on_scan_start(std::function< void()> on_scan_start)"
  brief="Set callback for scan start"
  detailed="This callback will be invoked when scanning starts..."
  parameters={[
    {
      name: "on_scan_start",
      type: "std::function< void()>",
      description: "Callback function to invoke"
    }
  ]}
/>
```

## Características de Estilo

### ApiSection
- Icono decorativo (Layers)
- Borde inferior destacado
- Espaciado generoso entre secciones

### ApiClass
- Card con gradiente sutil
- Header con icono (Box) y nombre en fuente monoespaciada
- Borde redondeado y sombra
- Área de descripción con fondo ligeramente diferenciado

### ApiMethod
- Card compacto con hover effects
- Icono de código (Code2)
- Firma en fuente monoespaciada
- Sección de parámetros con estilo diferenciado
- Animación sutil al hacer hover

## Generación Automática

Estos componentes se utilizan automáticamente por el script `scripts/build_api_docs.mjs`, que:

1. Ejecuta Doxygen para generar XML
2. Parsea el XML con `fast-xml-parser`
3. Genera MDX usando estos componentes React
4. Escribe `content/docs/simpleble/api.mdx`

No es necesario escribir MDX manualmente; el parser lo genera automáticamente desde el código C++ comentado con Doxygen.

## Personalización

Para personalizar los estilos, edita los archivos en `src/components/api/`:
- `ApiSection.tsx` - Estilo de secciones principales
- `ApiClass.tsx` - Estilo de clases
- `ApiMethod.tsx` - Estilo de métodos

Todos los componentes usan:
- Tailwind CSS para estilos
- `lucide-react` para iconos
- Variables de tema de Fumadocs (`fd-*` classes)
