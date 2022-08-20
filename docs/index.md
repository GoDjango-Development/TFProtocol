# Django Payments
Django Payments is a wrapper around many payment methods for django...
## How to use
Configure your settings as follows:

```py settings.py
def amount_generator_example(request: HttpRequest, *caller_view_args, **caller_view_kwargs):
    return request.session.get("cart_object", {}).get("total", 0)
def access_token_generator_example(request: HttpRequest, *caller_view_args, **caller_view_kwargs):
    return caller_view_kwargs.get("user").get("access_token", "access_token$sandbox$youraccesstoken")

INSTALLED_PLUGINS = {
    PLUGIN_NAME: {
        "version": "1.0.0",
        "agreement": "Agreement from settings",
        "amount": amount_generator_example, # Also could be a lambda
        "access_token": access_token_generator_example, # Also could be a lambda
        "context": {
            "urls_kwargs": {
                "var_identifier": lambda context: {
                    "url_var_for_reverser": "any data you want"
                }
            }
        }
    }
}
```
* version: This is just for later use not really important right now.
* agreement: Is shown in the PayPal form when paying. 
* amount: The dynamic amount to pay. It must be a callable. # Required
* access_token: The User PayPal account Access Token. It must be a callable.  # Required
* context: A dict containing all context data to be used in the ecosystem, it supports:
    * urls_kwargs: A dict container an key/value with kwargs to be used in intern django.urls.reverse calls, the value it is a 
    function that accept at least an argument of type django.template.context.Context, this allows user to determine some things
    about the current template and in response give a custom url kwarg 

In the template use as follows:
```py template.html

{% include 'paypal_buttons.html' with total="0.2" auth_token="access_token$sandbox$youraccesstoken" context_id="same_that_in_your_settings" %}

```
