/* SPDX-License-Identifier: Zlib */

#include "plugin.h"
#include "utils.h"

zathura_error_t
pdf_document_open(zathura_document_t* document)
{
  if (document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  /* format path */
  GError* gerror  = NULL;
  char* file_uri = g_filename_to_uri(zathura_document_get_path(document), NULL, NULL);

  if (file_uri == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  PopplerDocument* poppler_document = poppler_document_new_from_file(file_uri,
      zathura_document_get_password(document), &gerror);

  if (poppler_document == NULL) {
    if (gerror != NULL && gerror->code == POPPLER_ERROR_ENCRYPTED) {
      error = ZATHURA_ERROR_INVALID_PASSWORD;
    } else {
      error = ZATHURA_ERROR_UNKNOWN;
    }
    goto error_free;
  }

  zathura_document_set_data(document, poppler_document);

  int n_pages = poppler_document_get_n_pages(poppler_document);
  zathura_document_set_number_of_pages(document, n_pages);

  g_free(file_uri);


  // signature stuff
  if (g_signature_overlay_toggle == true) {
    g_signature_array = g_ptr_array_sized_new(n_pages);
    printf("allocating sig array at %p\nlen is %d\n", (void*)g_signature_array, g_signature_array->len);

    for (int page_i = 0; page_i < n_pages; page_i++) {
      //g_ptr_array_add(g_signature_array, NULL);
      g_ptr_array_add(g_signature_array, g_ptr_array_new());
      printf("    allocated page sig array at %p\n", g_ptr_array_index(g_signature_array, page_i));
    }
    printf("  len is now %d\n", g_signature_array->len);

    check_signatures(poppler_document);
  }


  return ZATHURA_ERROR_OK;

error_free:

  if (gerror != NULL) {
    g_error_free(gerror);
  }

  if (file_uri != NULL) {
    g_free(file_uri);
  }

  return error;
}

zathura_error_t
pdf_document_free(zathura_document_t* document, void* data)
{
  if (document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  PopplerDocument* poppler_document = data;
  if (poppler_document != NULL) {
    g_object_unref(poppler_document);
    zathura_document_set_data(document, NULL);
  }

  // free signature stuff
  if (g_signature_overlay_toggle == true) {
    printf("freeing\n");

    for (guint page_i = 0; page_i < g_signature_array->len; page_i++) {
      GPtrArray* page_signature_array = g_ptr_array_index(g_signature_array, page_i);

      if (page_signature_array != NULL) {
        printf("  page sig array %p for page %d has len %d\n", (void*)page_signature_array, page_i, page_signature_array->len);

        for (guint sig_i = 0; sig_i < page_signature_array->len; sig_i++) {
          PopplerSignatureInfo* sig_info = (PopplerSignatureInfo*) g_ptr_array_index(page_signature_array, sig_i);
          printf("    freeing sig info at %p\n", (void*)sig_info);
          poppler_signature_info_free(sig_info);
        }
        printf("  freeing page sig array at %p\n", (void*)page_signature_array);
        g_ptr_array_unref(page_signature_array);
      }
      else {
        printf("  pagesigarray for index %d is NULL -> should not happen\n", page_i);
      }
    }
    
    printf("freeing sig array at %p\n", (void*)g_signature_array);
    g_ptr_array_unref(g_signature_array);
  }

  return ZATHURA_ERROR_OK;
}

zathura_error_t
pdf_document_save_as(zathura_document_t* document, void* data, const char* path)
{
  if (document == NULL || data == NULL || path == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* format path */
  char* file_uri = g_filename_to_uri(path, NULL, NULL);
  if (file_uri == NULL) {
    return ZATHURA_ERROR_UNKNOWN;
  }

  PopplerDocument* poppler_document = data;

  const gboolean ret = poppler_document_save(poppler_document, file_uri, NULL);
  g_free(file_uri);

  return (ret == TRUE ? ZATHURA_ERROR_OK : ZATHURA_ERROR_UNKNOWN);
}
