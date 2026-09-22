namespace SimpleSharpBLE.Internal;

// Mono's AOT compiler recognizes this attribute by name, without an Apple SDK dependency.
[AttributeUsage(AttributeTargets.Method)]
internal sealed class MonoPInvokeCallbackAttribute : Attribute
{
    public MonoPInvokeCallbackAttribute(Type delegateType) { }
}
